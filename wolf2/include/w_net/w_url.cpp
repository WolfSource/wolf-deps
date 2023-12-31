#include "w_system_pch.h"
#include "w_url.h"
#include "curl/curl.h"

namespace wolf
{
	namespace system
	{
		class w_url_pimp
		{
		public:
			w_url_pimp() :
				_curl(nullptr)
			{
				this->_curl = curl_easy_init();
				if (!this->_curl)
				{
					logger.error("could not initialize curl");
					return;
				}
			}

			W_RESULT request_url(
				_In_z_ const char* pURL, 
				_Inout_ std::string& pResultPage, 
				_In_ w_point pAbortIfSlowerThanNumberOfBytesInSeconds,
				_In_ const uint32_t& pConnectionTimeOutInMilliSeconds)
			{
                if (!this->_curl) return W_FAILED;

                //reset memory
                _chunk.reset();

                curl_easy_setopt(this->_curl, CURLOPT_URL, pURL);
                curl_easy_setopt(this->_curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(this->_curl, CURLOPT_VERBOSE, 1L);
                curl_easy_setopt(this->_curl, CURLOPT_CONNECTTIMEOUT_MS, pConnectionTimeOutInMilliSeconds);
                curl_easy_setopt(this->_curl, CURLOPT_ACCEPTTIMEOUT_MS, pConnectionTimeOutInMilliSeconds);
                curl_easy_setopt(this->_curl, CURLOPT_WRITEFUNCTION, _write_callback);
                curl_easy_setopt(this->_curl, CURLOPT_WRITEDATA, (void*)(&this->_chunk));
                //some servers don't like requests that are made without a user-agent field, so we provide one
                curl_easy_setopt(this->_curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
                // abort if slower than bytes/sec 
				curl_easy_setopt(this->_curl, CURLOPT_LOW_SPEED_LIMIT, pAbortIfSlowerThanNumberOfBytesInSeconds.x);
				curl_easy_setopt(this->_curl, CURLOPT_LOW_SPEED_TIME, pAbortIfSlowerThanNumberOfBytesInSeconds.y);
                
				//set the default protocol
				//curl_easy_setopt(this->_curl, CURLOPT_DEFAULT_PROTOCOL, "https");

                //perform the request
                auto _result = curl_easy_perform(this->_curl);

                //check for errors
                W_RESULT _hr = W_PASSED;
                if (_result == CURLE_OK)
                {
                    _chunk.copyto(pResultPage);
                }
                else
                {
                    logger.error(
                        "could not get result of requested url : {} because {}",
                        pURL, curl_easy_strerror(_result));
                    _hr = W_FAILED;
                }

                _chunk.reset();

                return _hr;
			}

			W_RESULT send_rest_post(
				_In_z_ const char* pURL, 
				_In_z_ const char* pMessage, 
				_In_ const size_t& pMessageLenght, 
				_Inout_ std::string& pResult,
				_In_ w_point& pAbortIfSlowerThanNumberOfBytesInSeconds,
				_In_ const uint32_t& pConnectionTimeOutInMilliSeconds,
				_In_z_ std::initializer_list<std::string> pHeaders)
			{
                if (!this->_curl) return W_FAILED;

                //reset memory
                _chunk.reset();

                //set POST url
                curl_easy_setopt(this->_curl, CURLOPT_URL, pURL);
                //now specify the POST data
                curl_easy_setopt(this->_curl, CURLOPT_POSTFIELDS, pMessage);
                curl_easy_setopt(this->_curl, CURLOPT_POSTFIELDSIZE, pMessageLenght);
                curl_easy_setopt(this->_curl, CURLOPT_POST, 1L);
                curl_easy_setopt(this->_curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(this->_curl, CURLOPT_WRITEFUNCTION, _write_callback);
                curl_easy_setopt(this->_curl, CURLOPT_WRITEDATA, (void*)(&this->_chunk));
                curl_easy_setopt(this->_curl, CURLOPT_CONNECTTIMEOUT_MS, pConnectionTimeOutInMilliSeconds);
                curl_easy_setopt(this->_curl, CURLOPT_ACCEPTTIMEOUT_MS, pConnectionTimeOutInMilliSeconds);
                // abort if slower than bytes/sec 
				curl_easy_setopt(this->_curl, CURLOPT_LOW_SPEED_LIMIT, pAbortIfSlowerThanNumberOfBytesInSeconds.x);
				curl_easy_setopt(this->_curl, CURLOPT_LOW_SPEED_TIME, pAbortIfSlowerThanNumberOfBytesInSeconds.y);

                //set http header
                //for example "Accept: application/json";
                struct curl_slist* _headers = NULL;
                for (auto _header : pHeaders)
                {
                    _headers = curl_slist_append(_headers, _header.c_str());
                }
                curl_easy_setopt(this->_curl, CURLOPT_HTTPHEADER, _headers);
                //perform the request
                auto _result = curl_easy_perform(this->_curl);

                //free chuck
                curl_slist_free_all(_headers);

                //check for errors
                if (_result != CURLE_OK)
                {
                    logger.error(
                        "could not get result of requested rest post : {} / {} because {}",
                        pURL, pMessage,  curl_easy_strerror(_result));
                    return W_FAILED;
                }

                _chunk.copyto(pResult);
                _chunk.reset();

				return W_PASSED;
			}

			const std::string encoded_URL(_In_z_ const std::string& pURL)
			{
				std::string _output_str;
				if (!_curl) return _output_str;

				char* _output = curl_easy_escape(this->_curl, pURL.c_str(), pURL.size());
				if (_output) 
				{
					_output_str = _output;
					curl_free(_output);
				}

				return _output_str;
			}

			void release()
			{
				if (this->_curl)
				{
					//curl_easy_cleanup(this->_curl);
					free(this->_chunk.memory);
					this->_curl = nullptr;
				}
			}

		private:
			static size_t _write_callback(
				void* pContents,
				size_t pSize,
				size_t pNmemb,
				void* pUserp)
			{
				size_t _real_size = pSize * pNmemb;
				struct url_memory* _mem = (struct url_memory*)pUserp;

				auto _ptr = (char*)realloc(_mem->memory, _mem->size + _real_size + 1);
				if (_ptr == NULL)
				{
					//out of memory
					wolf::logger.error("could not allocate memory for result of url");
					return 0;
				}

				_mem->memory = _ptr;
				memcpy(&(_mem->memory[_mem->size]), pContents, _real_size);
				_mem->size += _real_size;
				_mem->memory[_mem->size] = 0;

				return _real_size;
			}

			CURL*									_curl;
			struct url_memory
			{
				char*	memory = nullptr;
				size_t	size = 0;

				void reset()
				{
					this->size = 0;
					if (this->memory)
					{
						free(this->memory);
					}
					this->memory = (char*)malloc(1);//will be grown as needed by the realloc above
				}

				void copyto(_Inout_ std::string& pDestination)
				{
					if (this->size && this->memory)
					{
						pDestination.resize(this->size);
						memcpy(&pDestination[0], &this->memory[0], this->size);
					}
				}

			} _chunk;
		};
	}
}

using namespace wolf::system;

static std::once_flag _once_init;
static std::once_flag _once_release;

w_url::w_url() :
	_is_released(false),
	_pimp(new w_url_pimp())
{
	std::call_once(_once_init, [&]()
		{
			curl_global_init(CURL_GLOBAL_ALL);
		});
}

w_url::~w_url()
{
	release();
}

W_RESULT w_url::request_url(_In_z_ const std::string& pURL, 
	_Inout_ std::string& pResultPage,
	_In_ w_point& pAbortIfSlowerThanNumberOfBytesInSeconds,
	_In_ const uint32_t& pConnectionTimeOutInMilliSeconds)
{
	if (!this->_pimp) return W_FAILED;

	auto _size = pURL.size();
	auto _url = (char*)malloc( (pURL.size() + 1 ) * sizeof(char));
	if (!_url)
	{
		wolf::logger.error("could not allocate memory for url");
		return W_FAILED;
	}
	//copy data
	std::memcpy(&_url[0], pURL.data(), _size);
	_url[_size] = '\0';
	auto _hr = this->_pimp->request_url(
		_url, 
		pResultPage, 
		pAbortIfSlowerThanNumberOfBytesInSeconds,
		pConnectionTimeOutInMilliSeconds);
	//free(_url);
	return _hr;
}

const std::string w_url::encoded_URL(_In_z_ const std::string& pURL)
{
	if (!this->_pimp) return "";
	return this->_pimp->encoded_URL(pURL);
}

W_RESULT w_url::send_rest_post(
	_In_z_ const std::string& pURL,
	_In_z_ const std::string& pMessage,
	_In_ const size_t& pMessageLenght,
	_Inout_ std::string& pResult,
	_In_ w_point& pAbortIfSlowerThanNumberOfBytesInSeconds,
	_In_ const uint32_t& pConnectionTimeOutInMilliSeconds,
	_In_z_ std::initializer_list<std::string> pHeaders)
{
	if (!this->_pimp) return W_FAILED;

	//copy url data
	auto _size = pURL.size();
	auto _url = (char*)malloc(pURL.size() * sizeof(char));
	if (!_url)
	{
		wolf::logger.error("could not allocate memory for url");
		return W_FAILED;
	}
	std::memcpy(&_url[0], pURL.data(), _size);
	_url[_size] = '\0';

	//copy message data
	_size = pMessage.size();
	auto _msg = (char*)malloc( (pMessage.size() + 1) * sizeof(char));
	if (!_msg)
	{
		wolf::logger.error("could not allocate memory for url");
		return W_FAILED;
	}
	std::memcpy(&_msg[0], pMessage.data(), _size);
	_msg[_size] = '\0';

	auto _hr = this->_pimp->send_rest_post(
		_url,
		_msg,
		pMessageLenght,
		pResult,
		pAbortIfSlowerThanNumberOfBytesInSeconds,
		pConnectionTimeOutInMilliSeconds,
		pHeaders);
	//free(_url);

	return _hr;
}

ULONG w_url::release()
{
	if (this->_is_released) return 1;

	SAFE_RELEASE(this->_pimp);

	this->_is_released = true;
	return 0;
}
