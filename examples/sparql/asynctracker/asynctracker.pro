include(../sparql-examples.pri)

SOURCES      += main.cpp
HEADERS      += main.cpp
#QT           += sparql # enable this later

# install # FIXME
target.path = $$[QT_INSTALL_EXAMPLES]/sparql/asynctracker
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS asynctracker.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/sparql/asynctracker
INSTALLS += target sources

