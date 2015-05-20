#include <senseboard2015.h>

extern "C" {
void* getInstance () {
	return new Senseboard2015();
}
}
