# Joins the PORT_OVERRIDE lists of port/src (PORT) and expanded/src (EXPANDED) into OUT with LF line
# ends, rewriting it only when it changes so ps2/src rebuilds only when an override comes or goes.
file(READ ${PORT} port_list)
file(READ ${EXPANDED} expanded_list)
set(merged "${port_list}${expanded_list}")
file(CONFIGURE OUTPUT ${OUT} CONTENT "${merged}" @ONLY NEWLINE_STYLE LF)
