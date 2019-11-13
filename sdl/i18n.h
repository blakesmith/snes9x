#pragma once

#include <string>
#include <vector>

struct Lang {
	std::string locale;
	std::string name;
};

class I18n {
public:
	void init(const std::string &appname);
	void apply(const std::string &lang);
    inline const std::vector<Lang> &getList() { return languages_; }

private:
	std::vector<Lang> languages_;
};
