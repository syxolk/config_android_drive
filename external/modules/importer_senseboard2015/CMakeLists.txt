include_directories("include")
set ( SOURCES 
	"src/senseboard2015.cpp"
	"src/converter.cpp"
	"src/interface.cpp"
	"src/comm.cpp"
        "src/crc.cpp"
)

set (HEADERS 
	"include/senseboard2015.h"
        "include/converter.h"
        "include/comm/senseboard.h"
        "include/comm/comm.h"
        "include/comm/protocol.h"
)

#TODO Brauchen wir glaube ich nicht mehr link_libraries(hardware_comm)
add_library ( importer_senseboard2015 SHARED ${SOURCES} ${HEADERS})

target_link_libraries(importer_senseboard2015 PRIVATE lms)
