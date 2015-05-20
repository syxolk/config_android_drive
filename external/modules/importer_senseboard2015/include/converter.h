#ifndef SENSEBOARD2015_CONVERTER_H
#define SENSEBOARD2015_CONVERTER_H


#include <string>
#include <map>
#include <stdint.h>
#include <lms/type/module_config.h>
//#include <core/configfile.h>

class Converter {
public:
	~Converter();

    void config(const lms::type::ModuleConfig *config);

	template <typename from, typename to>
	to convert(const from &value) {
		switch(mode) {
			case MODE_LINEAR:
				return c.linear.factor * value + c.linear.offset;
			case MODE_MAPPING:
				return (*c.mapping.map)[value];
			case MODE_IR_CURVE:
				return 0; //TODO
			case MODE_DUAL_LINEAR:
				if (value < 0)
					return c.dual_linear.factor_positive * value + c.dual_linear.offset_positive;
				else
					return -c.dual_linear.factor_negative * value + c.dual_linear.offset_negative;
			default:
				printf("Unknown Conversion Mode %i\n", mode);
				return -1;
		}
		return -1;
	}

	template <typename from, typename to>
	from reverse (const to &value) {
		switch (mode) {
		case MODE_LINEAR:
			return (value - c.linear.offset) / c.linear.factor;
		case MODE_MAPPING:
			for(auto it = c.mapping.map->begin(); it != c.mapping.map->end(); ++it) {
				if (it->second == value)
					return it->first;
			}
			return -1;
		case MODE_IR_CURVE:
			return 0; //TODO
		case MODE_DUAL_LINEAR:
			return -1;

		}
		return -1;
	}
private:
	enum conversionMode {
		MODE_LINEAR, MODE_IR_CURVE, MODE_MAPPING, MODE_DUAL_LINEAR
	} mode;


    void parse_config_linear(const lms::type::ModuleConfig *config);
    void parse_config_ir_curve(const lms::type::ModuleConfig *config);
    void parse_config_mapping(const lms::type::ModuleConfig *config);
    void parse_config_dual_linear(const lms::type::ModuleConfig *config);

	union {
		struct {
			float factor;
			float offset;
		} linear;

		struct {
			float factor_positive;
			float factor_negative;
			float offset_positive;
			float offset_negative;
		} dual_linear;

		struct {
			float peek;
			float min;
			float max;
		} ir_curve;

		struct {
			std::map<int16_t,float> *map;
		} mapping;
	} c;
};


#endif

