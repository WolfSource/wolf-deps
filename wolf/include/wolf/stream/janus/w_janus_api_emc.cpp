#if defined(WOLF_STREAM_JANUS) && defined(EMSCRIPTEN)

#include "w_janus_api_emc.hpp"
#include <cstdio>
#include <emscripten/emscripten.h>

using w_janus_api_emc = wolf::stream::janus::w_janus_api_emc;

static std::function<void(int)> s_on_init_callback = nullptr;
static std::function<void(double)> s_on_screen_shared_callback = nullptr;
static std::function<void(void)> s_on_join_room_callback = nullptr;

extern "C" {
EMSCRIPTEN_KEEPALIVE
void on_init_callback(int p_init_state) {
  if (s_on_init_callback) {
    s_on_init_callback(p_init_state);
  }
}

EMSCRIPTEN_KEEPALIVE
void on_screen_shared(_In_ double p_room_id) {
  if (s_on_screen_shared_callback) {
    s_on_screen_shared_callback(p_room_id);
  }
}

EMSCRIPTEN_KEEPALIVE
void on_join_room_callback() {
  if (s_on_join_room_callback) {
    s_on_join_room_callback();
  }
}
}

// clang-format off

boost::leaf::result<int>
w_janus_api_emc::init(_In_ bool p_debug_mode,
                      _In_ std::function<void(int)> p_on_init_callback) noexcept {

  s_on_init_callback = std::move(p_on_init_callback);

  return EM_ASM_INT({
    var _debug = "";
    
    if ($0 === 1) {
      _debug = "all";
    }
    // init Janus
    Janus.init({
      debug: _debug , callback: function () {
        // Make sure the browser supports WebRTC
        if (!Janus.isWebrtcSupported()) {
          console.error("No WebRTC support");
          return 1;
        }
      }
    });

    // global variables
    capture = null;
    id = null;
    local_tracks = {};
    local_videos = 0;
    remote_tracks = {};
    remote_videos = 0;
    role = null;
    room = null;
    screen = null;
    source = null;
    spinner = null;

    // generate random opaque id
    var _char_set = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
	  opaque_id = "";
	  for (var i = 0; i < 11; i++) {
		  var _random_pos = Math.floor(Math.random() * _char_set.length);
		  opaque_id += _char_set.substring(_random_pos, _random_pos + 1);
	  }

    
    // create a session
		janus = new Janus({
				server: server,
				iceServers: ice_servers,
				success: function () {
					// attach to videoroom plugin
					janus.attach(
						{
							plugin: "janus.plugin.videoroom",
							opaqueId: opaque_id,
							success: function (p_plugin_handle) {
								screen = p_plugin_handle;
								Janus.log("plugin attached! (" + screen.getPlugin() + ", id=" + screen.getId() + ")");
								_on_init_callback(0);
							},
							error: function (p_error) {
								Janus.error("error attaching plugin: ", p_error);
								_on_init_callback(1);
							},
							consentDialog: function (p_on) {
								Janus.debug("consent dialog should be " + (p_on ? "on" : "off") + " now");
							},
							iceState: function (p_state) {
								Janus.log("ICE state changed to " + p_state);
							},
							mediaState: function (p_medium, p_on, p_mid) {
								Janus.log("Janus " + (p_on ? "started" : "stopped") + " receiving our " + p_medium + " (mid=" + p_mid + ")");
							},
							webrtcState: function (p_on) {
								Janus.log("Janus WebRTC peer connection is " + (p_on ? "up" : "down") + " now");
								$("#streamscreen").parent().parent().unblock();
								if (p_on) {
									Janus.log("your screen sharing session just started: pass the \"" + room + "\" session identifier to those who want to attend.");
									_on_screen_shared(room);
								} else {
									Janus.log("your screen sharing session just stopped");
									janus.destroy();
								}
							},
							slowLink: function (p_uplink, p_lost, p_mid) {
								Janus.log("Janus reports problems " + (p_uplink ? "sending" : "receiving") + " packets on mid " + p_mid + " (" + p_lost + " lost packets)");
							},
							onmessage: function (p_msg, p_jsep) {
								Janus.debug("got a message (publisher): ", p_msg);
								var _event = p_msg["videoroom"];
								Janus.debug("event: " + _event);
								if (_event) {
									if (_event === "joined") {
										id = p_msg["id"];
										Janus.log("successfully joined room " + p_msg["room"] + " with ID " + id);
										if (role === "publisher") {
											// this is our session, publish our stream
											Janus.debug("negotiating WebRTC stream for our screen (capture " + capture + ")");
											// safari expects a user gesture to share the screen
											if (Janus.webRTCAdapter.browserDetails.browser === "safari") {
												window.alert("Safari requires a user gesture before the screen can be shared: close this dialog to do that");
												screen.createOffer(
													{
														// we want to capture the screen and audio, but sendonly
														tracks: [
															{ type: 'audio', capture: true, recv: false },
															{ type: 'screen', capture: true, recv: false }
														],
														success: function (p_jsep) {
															Janus.debug("Got publisher SDP!", p_jsep);
															var _publish = { request: "configure", audio: true, video: true };
															screen.send({ message: _publish, jsep: p_jsep });
														},
														error: function (p_error) {
															Janus.error("WebRTC error:", p_error.message);
														}
													});
											} else {
												// other browsers should be fine, we try to call getDisplayMedia directly
												screen.createOffer(
													{
														// we want sendonly audio and screensharing
														tracks: [
															{ type: 'audio', capture: true, recv: false },
															{ type: 'screen', capture: true, recv: false }
														],
														success: function (p_jsep) {
															Janus.debug("got publisher SDP!", p_jsep);
															var _publish = { request: "configure", audio: true, video: true };
															screen.send({ message: _publish, jsep: p_jsep });
														},
														error: function (p_error) {
															Janus.error("WebRTC error:", p_error);
														}
													});
											}
										} else {
											// we're just watching a session, any feed to attach to?
											if (p_msg["publishers"]) {
												var _list = p_msg["publishers"];
												Janus.debug("got a list of available publishers/feeds:", _list);
												for (var f in _list) {
													if (_list[f]["dummy"]) continue;
													var _id = _list[f]["id"];
													var _display = _list[f]["display"];
													Janus.debug("  >> [" + _id + "] " + _display);
													new_remote_feed(_id, _display)
												}
											}
										}
									} else if (_event === "event") {
										// any feed to attach to?
										if (role === "listener" && p_msg["publishers"]) {
											var _list = p_msg["publishers"];
											Janus.debug("got a list of available publishers/feeds:", _list);
											for (var f in _list) {
												if (_list[f]["dummy"]) continue;
												var _id = _list[f]["id"];
												var _display = _list[f]["display"];
												Janus.debug("  >> [" + _id + "] " + _display);
												new_remote_feed(_id, _display)
											}
										} else if (p_msg["leaving"]) {
											// one of the publishers has gone away?
											var _leaving = p_msg["leaving"];
											Janus.log("publisher left: " + _leaving);
											if (role === "listener" && p_msg["leaving"] === source) {
												Janus.error("the screen sharing session is over, the publisher left");
											}
										} else if (p_msg["error"]) {
											Janus.error(p_msg["error"]);
										}
									}
								}
								if (p_jsep) {
									Janus.debug("handling SDP as well ", p_jsep);
									screen.handleRemoteJsep({ jsep: p_jsep });
								}
							},
							onlocaltrack: function (p_track, p_on) {
								Janus.debug("local track " + (p_on ? "added" : "removed") + ":", p_track);
								// we use the track ID as name of the element, but it may contain invalid characters
								var _track_id = p_track.id.replace(/[{}]/g, "");
								if (!p_on) {
									// track removed, get rid of the stream and the rendering
									var _stream = local_tracks[_track_id];
									if (_stream) {
										try {
											var _tracks = _stream.getTracks();
											for (var _t in _tracks) {
												var _mst = _tracks[_t];
												if (_mst) {
													_mst.stop();
												}
											}
										} catch (p_e) { }
									}
									if (p_track.kind === "video") {
										$('#screenvideo' + _track_id).remove();
										local_videos--;
										if (local_videos === 0) {
											// No video, at least for now: show a placeholder
											if ($('#streamscreen .no-video-container').length === 0) {
												Janus.log("No webcam available");
											}
										}
									}
									delete local_tracks[_track_id];
									return;
								}
								// if we're here, a new track was added
								var _stream = local_tracks[_track_id];
								if (_stream) {
									// we've been here already
									return;
								}
								if (p_track.kind === "audio") {
									// we ignore local audio tracks, they'd generate echo anyway
									if (local_videos === 0) {
										// No video, at least for now: show a placeholder
										if ($('#streamscreen .no-video-container').length === 0) {
											Janus.log("No webcam available");
										}
									}
								} else {
									// new video track: create a stream out of it
									local_videos++;
									$('#streamscreen .no-video-container').remove();
									_stream = new MediaStream([p_track]);
									local_tracks[_track_id] = _stream;
									Janus.log("Created local stream:", _stream);
									$('#streamscreen').append('<video muted="muted" id="screenvideo' + _track_id + '" width=100% autoplay playsinline muted="muted"/>');
									Janus.attachMediaStream($('#screenvideo' + _track_id).get(0), _stream);
								}
								if (screen.webrtcStuff.pc.iceConnectionState !== "completed" &&
									screen.webrtcStuff.pc.iceConnectionState !== "connected") {
									$("#streamscreen").parent().parent().block({
										message: '<b>Publishing...</b>',
										css: {
											border: 'none',
											backgroundColor: 'transparent',
											color: 'white'
										}
									});
								}
							},
							onremotetrack: function (track, mid, on) {
								// the publisher stream is sendonly, we don't expect anything here
							},
							oncleanup: function () {
								Janus.log("got a cleanup notification");
								$('#streamscreen').empty();
								$("#streamscreen").parent().unblock();
								local_tracks = {};
								local_videos = 0;
							}
						});
				},
				error: function (error) {
					Janus.error(error);
				},
				destroyed: function () {
					Janus.log("Janus session id:" + id + " destroyed");
				}
			});

      // create new_remote_feed function 
      new_remote_feed = function (p_id, p_display) {
	      // a new feed has been published, create a new plugin handle and attach to it as a listener
	      source = p_id;
	      var _remote_feed = null;
	      janus.attach({
	      		plugin: "janus.plugin.videoroom",
	      		opaqueId: opaque_id,
	      		success: function (p_plugin_handle) {
	      			_remote_feed = p_plugin_handle;
	      			_remote_feed.remote_tracks = {};
	      			_remote_feed.remote_videos = 0;
	      			Janus.log("plugin attached! (" + _remote_feed.getPlugin() + ", id=" + _remote_feed.getId() + ")");
	      			Janus.log("this is a subscriber");
	      			// we wait for the plugin to send us an offer
	      			var _listen = {
	      				request: "join",
	      				room: room,
	      				ptype: "subscriber",
	      				feed: p_id
	      			};
	      			_remote_feed.send({ message: _listen });
	      		},
	      		error: function (p_error) {
	      			Janus.error("error attaching plugin: ", p_error);
	      		},
	      		iceState: function (p_state) {
	      			Janus.log("ICE state (feed #" + _remote_feed.rfindex + ") changed to " + p_state);
	      		},
	      		webrtcState: function (p_on) {
	      			Janus.log("Janus says this WebRTC PeerConnection (feed #" + _remote_feed.rfindex + ") is " + (p_on ? "up" : "down") + " now");
	      		},
	      		slowLink: function (p_uplink, p_lost, p_mid) {
	      			Janus.warn("Janus reports problems " + (p_uplink ? "sending" : "receiving") +
	      				" packets on mid " + p_mid + " (" + p_lost + " lost packets)");
	      		},
	      		onmessage: function (p_msg, p_jsep) {
	      			Janus.debug("got a message (listener): ", p_msg);
	      			var _event = p_msg["videoroom"];
	      			Janus.debug("Janus event: " + _event);
	      			if (_event) {
	      				if (_event === "attached") {
	      					// subscriber created and attached
	      					if (!spinner) {
	      						var _target = document.getElementById('#streamscreen');
	      						spinner = new Spinner({ top: 100 }).spin(_target);
	      					} else {
	      						spinner.spin();
	      					}
	      					Janus.log("successfully attached to feed " + id + " (" + p_display + ") in room " + p_msg["room"]);
	      				}// else {
	      					// what has just happened
	      					//Janus.error("unexpected point of program. event: " + _event);
	      				//}
	      			}
	      			if (p_jsep) {
	      				Janus.debug("handling SDP as well: ", p_jsep);
	      				// Answer and attach
	      				_remote_feed.createAnswer(
	      					{
	      						jsep: p_jsep,
	      						// We only specify data channels here, as this way in
	      						// case they were offered we'll enable them. Since we
	      						// don't mention audio or video tracks, we autoaccept them
	      						// as recvonly (since we won't capture anything ourselves)
	      						tracks: [
	      							{ type: 'data' }
	      						],
	      						success: function (p_jsep) {
	      							Janus.debug("got SDP!", p_jsep);
	      							var _body = { request: "start", room: room };
	      							_remote_feed.send({ message: _body, jsep: p_jsep });
	      						},
	      						error: function (p_error) {
	      							Janus.error("WebRTC error:", p_error);
	      						}
	      					});
	      			}
	      		},
	      		onlocaltrack: function (p_track, p_on) {
	      			// the subscriber stream is receive-only, we don't expect anything here
	      		},
	      		onremotetrack: function (p_track, p_mid, p_on) {
	      			Janus.debug("remote track (mid=" + p_mid + ") " + (p_on ? "added" : "removed") + ":", p_track);
	      			if (!p_on) {
	      				// Track removed, get rid of the stream and the rendering
	      				$('#screenvideo' + p_mid).remove();
	      				if (p_track.kind === "video") {
	      					remote_videos--;
	      					if (remote_videos === 0) {
	      						// no video, at least for now: show a placeholder
	      						if ($('#streamscreen .no-video-container').length === 0) {
									Janus.log("no remote video available");
	      						}
	      					}
	      				}
	      				delete remote_tracks[p_mid];
	      				return;
	      			}
	      			// If we're here, a new track was added
	      			if (spinner !== undefined && spinner !== null) {
	      				spinner.stop();
	      				spinner = null;
	      			}
	      			if (p_track.kind === "audio") {
	      				// new audio track: create a stream out of it, and use a hidden <audio> element
	      				_stream = new MediaStream([p_track]);
	      				remote_tracks[p_mid] = _stream;
	      				Janus.log("created remote audio stream:", _stream);
	      				$('#streamscreen').append('<audio hidden id="screenvideo' + p_mid + '" playsinline/>');
	      				$('#screenvideo' + p_mid).get(0).volume = 0;
	      				Janus.attachMediaStream($('#screenvideo' + p_mid).get(0), _stream);
	      				var _play_promise = $('#screenvideo' + p_mid).get(0).play();
						if (_play_promise !== undefined) {
  						  _play_promise.then(_ => {
  						    // play just started
							$('#screenvideo' + p_mid).get(0).volume = 1;
  						  })
  						  .catch(p_error => {
  						    //Janus.error(p_error);
  						  });
  						}
	      				if (remote_videos === 0) {
	      					// no video, at least for now: show a placeholder
	      					if ($('#streamscreen .no-video-container').length === 0) {
								Janus.log("no remote video available");
	      					}
	      				}
	      			} else {
	      				// new video track: create a stream out of it
	      				remote_videos++;
	      				$('#streamscreen .no-video-container').remove();
	      				_stream = new MediaStream([p_track]);
	      				_remote_feed.remote_tracks[p_mid] = _stream;
	      				Janus.log("created remote video stream:", _stream);
	      				$('#streamscreen').append('<video muted id="screenvideo' + p_mid + '" width=100% playsinline/>');
	      				$('#screenvideo' + p_mid).get(0).volume = 0;
	      				Janus.attachMediaStream($('#screenvideo' + p_mid).get(0), _stream);
	      				var _play_promise = $('#screenvideo' + p_mid).get(0).play();
						if (_play_promise !== undefined) {
  						  _play_promise.then(_ => {
  						    // play just started
							$('#screenvideo' + p_mid).get(0).volume = 1;
  						  })
  						  .catch(p_error => {
  						    //Janus.error(p_error);
  						  });
  						}
	      			}
	      		},
	      		oncleanup: function () {
	      			Janus.log("got a cleanup notification (remote feed " + p_id + ")");
	      			$('#waitingvideo').remove();
	      			if (spinner) {
	      				spinner.stop();
	      			}
	      			spinner = null;
	      			_remote_feed.remote_tracks = {};
	      			_remote_feed.remote_videos = 0;
	      		}
	      	});
      };

      return 0;
  }, (p_debug_mode ? 1 : 0));
}

boost::leaf::result<int> w_janus_api_emc::share_screen(
  _In_ const char* p_username,
  _In_ const char* p_room_desc,
  _In_ int p_bitrate,
  _In_ std::function<void(double)> p_on_success) noexcept {

  if(p_room_desc == nullptr || p_username == nullptr || p_bitrate <= 0){
    return W_FAILURE(std::errc::invalid_argument, "missing p_room_desc or p_username or p_bitrate is not valid");
  }

  s_on_screen_shared_callback = std::move(p_on_success);

  // clang-format off
  return EM_ASM_INT({
	// check for screensharing extension    
	if (!Janus.isExtensionEnabled()) {
		Janus.error("this browser doesn't support screensharing (getDisplayMedia unavailable)");
	    return;
	}
	capture = "screen";

	// create a new room
    var _desc = "" + UTF8ToString($1);
    role = "publisher";
    var create = {};
    create.request= "create";
    createdescription= _desc;
    create.bitrate= $2;
    createpublishers= 1;
    screen.send({
		message: create, success: function (p_result) {
			if (p_result["error"]) {
     			Janus.error("couldn't create room: " + p_result["error"]);
     			return;
     		}
     		var _event = p_result["videoroom"];
     		Janus.debug("Event: " + _event);
     		if (_event) {
     			// Our own screen sharing session has been created, join it
     			room = p_result["room"];
     			Janus.log("screen sharing session created: " + room);
     			var _username = "" + UTF8ToString($0);
     			var _register = {
     				request: "join",
     				room: room,
     				ptype: "publisher",
     				display: _username
		      	};
		    	screen.send({ message: _register });
	    	}
    	}
	});
    return 0;
  }, p_username, p_room_desc, p_bitrate);
}

boost::leaf::result<int> w_janus_api_emc::join_room(
	_In_ double p_room_id,
	_In_ const char* p_username,
    _In_ std::function<void(void)> p_on_success) noexcept
{
	if(p_room_id <= 0 || p_username == nullptr){
    	return W_FAILURE(std::errc::invalid_argument, "p_room_id or p_username is not valid");
  	}

	s_on_join_room_callback = std::move(p_on_success);

  	return EM_ASM_INT({
		// join an existing screen sharing session
		room = $0;
		role = "listener";
		var _username =  "" + UTF8ToString($1);
		var _register = {};
		_register.request= "join";
		_register.room= room;
		_register.ptype= "publisher";
		_register.display= _username;
		screen.send({ message: _register });
    	return 0;
  	}, p_room_id, p_username);
}

                // clang-format on

#endif // defined(WOLF_STREAM_JANUS) && defined(EMSCRIPTEN)
