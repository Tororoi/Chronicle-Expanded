# Joins the PORT_OVERRIDE lists of port/src (PORT) and expanded/src (EXPANDED) into OUT, rewriting
# it only when it changes so ps2/src rebuilds only when an override comes or goes.
file(READ ${PORT} port_list)
file(READ ${EXPANDED} expanded_list)
set(merged "${port_list}${expanded_list}")
if(EXISTS ${OUT})
    file(READ ${OUT} current)
endif()
if(NOT "${merged}" STREQUAL "${current}")
    file(WRITE ${OUT} "${merged}")
endif()
