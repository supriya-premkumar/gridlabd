pkglib_LTLIBRARIES += interconnection/interconnection.la

interconnection_interconnection_la_CPPFLAGS = -I/usr/local/include
interconnection_interconnection_la_CPPFLAGS += $(MATLAB_CPPFLAGS)
interconnection_interconnection_la_CPPFLAGS += $(AM_CPPFLAGS)

interconnection_interconnection_la_LDFLAGS = -L/usr/local/lib
interconnection_interconnection_la_LDFLAGS += $(MATLAB_LDFLAGS)
interconnection_interconnection_la_LDFLAGS += $(AM_LDFLAGS) -larmadillo

interconnection_interconnection_la_LIBADD = 
interconnection_interconnection_la_LIBADD += $(MATLAB_LIBS) 

interconnection_interconnection_la_SOURCES =
#interconnection_interconnection_la_SOURCES += interconnection/linalg.cpp
#interconnection_interconnection_la_SOURCES += interconnection/linalg.h
interconnection_interconnection_la_SOURCES += interconnection/solver.cpp
interconnection_interconnection_la_SOURCES += interconnection/solver.h
interconnection_interconnection_la_SOURCES += interconnection/weather.cpp
interconnection_interconnection_la_SOURCES += interconnection/weather.h
interconnection_interconnection_la_SOURCES += interconnection/dispatcher.cpp
interconnection_interconnection_la_SOURCES += interconnection/dispatcher.h
interconnection_interconnection_la_SOURCES += interconnection/generator.cpp
interconnection_interconnection_la_SOURCES += interconnection/generator.h
interconnection_interconnection_la_SOURCES += interconnection/load.cpp
interconnection_interconnection_la_SOURCES += interconnection/load.h
interconnection_interconnection_la_SOURCES += interconnection/intertie.cpp
interconnection_interconnection_la_SOURCES += interconnection/intertie.h
interconnection_interconnection_la_SOURCES += interconnection/controlarea.cpp
interconnection_interconnection_la_SOURCES += interconnection/controlarea.h
interconnection_interconnection_la_SOURCES += interconnection/scheduler.cpp
interconnection_interconnection_la_SOURCES += interconnection/scheduler.h
interconnection_interconnection_la_SOURCES += interconnection/interconnection.cpp
interconnection_interconnection_la_SOURCES += interconnection/interconnection.h
interconnection_interconnection_la_SOURCES += interconnection/transactive.h
interconnection_interconnection_la_SOURCES += interconnection/main.cpp
