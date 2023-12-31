#include "w_referee.hpp"

#include <chrono>
#include <opencv2/opencv.hpp>

using w_referee = wolf::ml::ocr::w_referee;
using w_ocr_engine = wolf::ml::ocr::w_ocr_engine;

w_referee::w_referee() {}

void w_referee::match_result_struct::release() {
  match_result_struct::result_image.release();
}

w_ocr_engine::character_and_center w_referee::concatenate_name_result(
    std::vector<w_ocr_engine::character_and_center> &result) {
  w_ocr_engine::character_and_center temp_concatenated_result = result[0];

  for (size_t i = 1; i < result.size(); i++) {
    temp_concatenated_result.text =
        temp_concatenated_result.text + " " + result[i].text;
  }

  return temp_concatenated_result;
}

auto w_referee::if_the_string_is_in_the_vector(
    w_ocr_engine::character_and_center the_character,
    std::vector<vote_over_string_vector> &the_vector) -> bool {
  bool is_in_the_vector = false;

  for (size_t i = 0; i < the_vector.size(); i++) {
    // std::cout << the_string.c_str() << " : " << the_vector[i].str.c_str() <<
    // "  " << std::strcmp(the_string.c_str(), the_vector[i].str.c_str()) <<
    // std::endl;
    if (std::strcmp(the_character.text.c_str(), the_vector[i].str.c_str()) ==
        0) {
      is_in_the_vector = true;
      the_vector[i].already_voted++;
      break;
    }
  }

  if (!is_in_the_vector) {
    vote_over_string_vector temp;
    temp.str = the_character.text;
    temp.center = the_character.center;
    the_vector.push_back(temp);
  }

  return is_in_the_vector;
}

void w_referee::voting_over_results_and_names(
    frame_result_struct &voted_results,
    std::vector<frame_result_struct> &all_results) {
  std::vector<vote_over_string_vector> home_result{};
  std::vector<vote_over_string_vector> away_result{};
  std::vector<vote_over_string_vector> home_name{};
  std::vector<vote_over_string_vector> away_name{};
  std::vector<vote_over_string_vector> home_penalty_result{};
  std::vector<vote_over_string_vector> away_penalty_result{};

  home_result.clear();
  away_result.clear();
  home_name.clear();
  away_name.clear();

  for (int j = 0; j < all_results.size(); j++) {
    if_the_string_is_in_the_vector(all_results[size_t(j)].home_result,
                                   home_result);
    if_the_string_is_in_the_vector(all_results[size_t(j)].away_result,
                                   away_result);
    if_the_string_is_in_the_vector(all_results[size_t(j)].home_name, home_name);
    if_the_string_is_in_the_vector(all_results[size_t(j)].away_name, away_name);
    if (all_results[size_t(j)].home_penalty_result.text.compare("") != 0) {
      if_the_string_is_in_the_vector(all_results[size_t(j)].home_penalty_result,
                                     home_penalty_result);
    }
    if (all_results[size_t(j)].away_penalty_result.text.compare("") != 0) {
      if_the_string_is_in_the_vector(all_results[size_t(j)].away_penalty_result,
                                     away_penalty_result);
    }
  }

  std::sort(home_result.begin(), home_result.end());
  std::sort(away_result.begin(), away_result.end());
  std::sort(home_name.begin(), home_name.end());
  std::sort(away_name.begin(), away_name.end());
  if (home_penalty_result.size() != 0 && away_penalty_result.size() != 0) {
    std::sort(home_penalty_result.begin(), home_penalty_result.end());
    std::sort(away_penalty_result.begin(), away_penalty_result.end());
    voted_results.home_penalty_result.text =
        home_penalty_result[home_penalty_result.size() - 1].str;
    voted_results.home_penalty_result.center =
        home_penalty_result[home_penalty_result.size() - 1].center;
    voted_results.away_penalty_result.text =
        away_penalty_result[away_penalty_result.size() - 1].str;
    voted_results.away_penalty_result.center =
        away_penalty_result[away_penalty_result.size() - 1].center;
  }
  voted_results.home_result.text = home_result[home_result.size() - 1].str;
  voted_results.home_result.center = home_result[home_result.size() - 1].center;
  voted_results.away_result.text = away_result[away_result.size() - 1].str;
  voted_results.away_result.center = away_result[away_result.size() - 1].center;
  voted_results.home_name.text = home_name[home_name.size() - 1].str;
  voted_results.home_name.center = home_name[home_name.size() - 1].center;
  voted_results.away_name.text = away_name[away_name.size() - 1].str;
  voted_results.away_name.center = away_name[away_name.size() - 1].center;

  voted_results.frame_number = all_results[0].frame_number;
  voted_results.stat = all_results[0].stat;

  return;
}
