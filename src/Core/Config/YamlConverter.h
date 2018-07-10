#ifndef YAMLCONVERTER_H
#define YAMLCONVERTER_H
#include <date/date.h>
#include <yaml-cpp/node/node.h>

namespace YAML {
  template <>
  struct convert<date::sys_days> {
    static Node encode(const date::sys_days& rhs) {
      Node node;
      node.push_back(date::format("%Y/%m/%d", rhs));
      return node;
    }

    static bool decode(const Node& node, date::sys_days& rhs) {

      if (!node.IsScalar()) {
        return false;
      }
      std::stringstream ss(node.as<std::string>());
      date::from_stream(ss, "%Y/%m/%d", rhs);
      return true;
    }
  };

  template <>
  struct convert<date::year_month_day> {
	  static Node encode(const date::year_month_day& rhs) {
		  Node node;
		  node.push_back(date::format("%Y/%m/%d", rhs));
		  return node;
	  }

	  static bool decode(const Node& node, date::year_month_day& rhs) {

		  if (!node.IsScalar()) {
			  return false;
		  }
		  std::stringstream ss(node.as<std::string>());
		  date::from_stream(ss, "%Y/%m/%d", rhs);
		  return true;
	  }
  };
}
#endif // YAMLCONVERTER_H
