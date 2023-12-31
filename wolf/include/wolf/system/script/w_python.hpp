#pragma once

#include <wolf.hpp>
#include <unordered_map>
#include <any>
#include <boost/python.hpp>

namespace wolf::system::python {

// a wrapper over boost.python
class w_python {
 public:
  W_API explicit w_python(const std::string_view& p_python_home_path);
  w_python(const w_python&) = delete;
  w_python& operator=(const w_python&) = delete;
  W_API ~w_python();

  /*
   * set python module
   * @param p_module_name, the module name
   */
  W_API void set_module(const std::string_view& p_module_name);

  /*
   * set python dictionary
   * @param p_dic, the dictionary
   */
  W_API void set_dictionary(const std::string& p_dic);
  /*
   * get python script
   * @return the script
   */
  W_API std::string get_script() const;

  /*
   * run an inline python script
   * @param p_script: python script
   * @return 0 if success, otherwise return error code
   */
  W_API boost::leaf::result<int> run(_In_ const std::string_view& p_script,
                                     std::unordered_map<std::string, std::any>& p_requested_vars);

 private:
  void _release();
  static std::string _parse_python_exception();

  std::string _script;
  std::string _module_name = "__main__";
  std::string _dictionary_name = "__dict__";

  boost::python::pointer_wrapper<boost::python::api::object> _global =
      boost::python::ptr(boost::python::api::object());
};

}  // namespace wolf::system::python
