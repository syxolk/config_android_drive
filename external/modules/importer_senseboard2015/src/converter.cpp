#include <converter.h>
#include <string>
#include <sstream>
#include <lms/type/module_config.h>

Converter::~Converter() {
	if (mode == MODE_MAPPING)
		delete c.mapping.map;
}

void Converter::config(const lms::type::ModuleConfig *config) {
    std::string m = config->get<std::string>("_conversion_mode", "linear");


	if ("linear" == m) {
		mode = MODE_LINEAR;
        parse_config_linear(config);
	} else if ("ir_curve" == m) {
		mode = MODE_IR_CURVE;
        parse_config_ir_curve(config);
	} else if ("dual_linear" == m) {
		mode = MODE_DUAL_LINEAR;
        parse_config_dual_linear(config);
	} else if ("mapping" == m) {
		mode = MODE_MAPPING;
		c.mapping.map = new std::map<int16_t, float>();
        parse_config_mapping(config);
	} else {
		fprintf(stderr, "\033[31mUnrecognized conversionmode %s. expecting one of linear, dual_linear, ir_curve or mapping\n", m.c_str());
	}
}


void Converter::parse_config_linear(const lms::type::ModuleConfig *cfg) {
    c.linear.factor = cfg->get<float>("factor", 1.0f);
    c.linear.offset = cfg->get<float>("offset", 0.0f);
}

void Converter::parse_config_ir_curve(const lms::type::ModuleConfig *cfg) {

	//TODO
}

void Converter::parse_config_dual_linear(const lms::type::ModuleConfig *cfg) {
    c.dual_linear.factor_positive = cfg->get<float>("positive_factor", 1.0f);
    c.dual_linear.offset_positive = cfg->get<float>("positive_offset", 0.0f);
    c.dual_linear.factor_negative = cfg->get<float>("negative_factor", 1.0f);
    c.dual_linear.offset_negative = cfg->get<float>("negative_offset", 0.0f);
}

void Converter::parse_config_mapping(const lms::type::ModuleConfig *cfg) {
    int count = cfg->get<int>("_num_keys", 1);

	for (int i = 0; i < count; ++i) {
		std::stringstream ss;
		ss << i;
        (*c.mapping.map)[cfg->get<int>(ss.str() + "_key", i)] = cfg->get<int>(ss.str() + "_value", 0);
	}
}
