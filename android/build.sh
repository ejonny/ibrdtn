#!/bin/sh -e
#

BUILD_MODE="release"
WORKSPACE=$(pwd)

### BUILD IBR-DTN API FOR ANDROID ###

# update android project
android update project -p ibrdtn-api -n android-ibrdtn-api

# build IBR-DTN API
ant -f ibrdtn-api/build.xml clean ${BUILD_MODE}

# copy API .jar to all DTN apps
cp ibrdtn-api/bin/classes.jar ibrdtn/libs/android-ibrdtn-api.jar
cp ibrdtn-api/bin/classes.jar Whisper/libs/android-ibrdtn-api.jar
cp ibrdtn-api/bin/classes.jar Talkie/libs/android-ibrdtn-api.jar
cp ibrdtn-api/bin/classes.jar DTNExampleApp/libs/android-ibrdtn-api.jar

### BUILD CHARTVIEW LIBRARY ###

# copy latest support library to chartview
cp ibrdtn/libs/android-support-v4.jar ChartView/library/libs

# update android project
android update project -p ChartView/library -n chartview

# build ChartView
ant -f ChartView/library/build.xml clean ${BUILD_MODE}

### BUILD IBR-DTN DAEMON ###

# build JNI part of the daemon
cd ibrdtn/jni
./build.sh

# switch back to workspace
cd ${WORKSPACE}

# update android project
android update project -p ibrdtn -n ibrdtn --library ../ChartView/library

# build IBR-DTN
ant -f ibrdtn/build.xml clean ${BUILD_MODE}

### BUILD WHISPER ###

# update android project
android update project -p Whisper -n Whisper

# build IBR-DTN API
ant -f Whisper/build.xml clean ${BUILD_MODE}

### BUILD TALKIE ###

# update android project
android update project -p Talkie -n Talkie

# build IBR-DTN API
ant -f Talkie/build.xml clean ${BUILD_MODE}

