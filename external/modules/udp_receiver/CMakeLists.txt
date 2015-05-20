set ( SOURCES 
	"src/udp_receiver.cpp"
	"src/interface.cpp"
)

set (HEADERS
	"include/udp_receiver.h"
)

include_directories("include")

add_library (udp_receiver MODULE ${SOURCES} ${HEADERS})
target_link_libraries(udp_receiver PRIVATE lms)
