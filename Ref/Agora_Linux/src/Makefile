.SECONDEXPANSION:

VERSION = 1.2.3
RECORDER_SDK = 0
ifeq ($(RECORDER_SDK), 1)
SDK_FLAGS = -DSERVER_RECORDER_SDK
endif

ifneq ($(JNI_INCLUDE_PATH),)
JNI_INCLUDE_FLAGS = -I$(JNI_INCLUDE_PATH) -I$(JNI_INCLUDE_PATH)/linux
endif

CC            = gcc
CXX           = g++
RELEASE_CFLAGS        = -pipe -O3 -fPIC -Wall -W -DNEDBUG=1 -g -fvisibility=hidden
RELEASE_CXXFLAGS      = -pipe -std=c++0x -fPIC -g -fno-omit-frame-pointer -DNDEBUG=1 \
												-DGIT_DESC=\"`git rev-parse HEAD`\" -O3 \
												-Wall -W  $(SDK_FLAGS) $(DEFINES) -fvisibility=hidden

DEBUG_CFLAGS        = -pipe -O0 -Wall -W -fPIC -g -fvisibility=hidden
DEBUG_CXXFLAGS      = -pipe -std=c++0x -fPIC -g -fno-omit-frame-pointer \
											-DGIT_DESC=\"`git rev-parse HEAD`\" -O0 \
											-Wall -W  $(SDK_FLAGS) $(DEFINES) -fvisibility=hidden

INCPATH       =  -I./include/ -Iinternal -I. $(JNI_INCLUDE_FLAGS)
LINK          = g++
# LDFLAGS        = -static-libgcc -static-libstdc++  # -Wl,-Bstatic,-lc -Wl,-Bstatic,-lm -Wl,-Bdynamic
AR            = ar cqs

OBJ_DIR = .objs
DEP_DIR = .deps
BIN_DIR = bin

BASE_SOURCES = $(wildcard base/*.cpp)
APPLITE_SOURCES = $(wildcard applite/*.cpp)
STUB_SOURCES = $(wildcard stub/*.cpp)
TEST_SOURCES = $(wildcard test/pseudo_pstn/*.cpp)
PERF_SOURCES = $(wildcard test/perf/*.cpp)
RECORDER_SOURCES = $(wildcard recorder/*.cpp)
RECORDING_SDK_SOURCES = $(wildcard recording_sdk/*.cpp)
PLAYER_SOURCES = $(wildcard player/*.cpp)
VIDEO_DEMO_SOURCES = $(wildcard video_recorder_demo/*.cpp)
JNI_SOURCES = $(wildcard jni/*.cpp)

TEST_JAVA_SOURCES = $(wildcard agora-rtc-server-java/*.java)

SOURCES = $(BASE_SOURCES) $(APPLITE_SOURCES) $(STUB_SOURCES) $(TEST_SOURCES) $(RECORDER_SOURCES) $(VIDEO_DEMO_SOURCES)

DBG_OBJ_PATH = debug/$(OBJ_DIR)
DBG_DEP_PATH = debug/$(DEP_DIR)
DBG_BIN_PATH = debug/$(BIN_DIR)
DBG_BASE_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(BASE_SOURCES))))
DBG_APPLITE_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(APPLITE_SOURCES))))
DBG_VIDEO_DEMO_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(VIDEO_DEMO_SOURCES))))
DBG_STUB_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(STUB_SOURCES))))
DBG_TEST_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(TEST_SOURCES))))
DBG_RECORDER_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(RECORDER_SOURCES))))
DBG_RECORDING_SDK_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(RECORDING_SDK_SOURCES))))
DBG_PLAYER_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(PLAYER_SOURCES))))
DBG_JNI_OBJECTS = $(addprefix $(DBG_OBJ_PATH)/, $(addsuffix .o, $(basename $(JNI_SOURCES))))

RLS_OBJ_PATH = release/$(OBJ_DIR)
RLS_DEP_PATH = release/$(DEP_DIR)
RLS_BIN_PATH = release/$(BIN_DIR)
RLS_BASE_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(BASE_SOURCES))))
RLS_APPLITE_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(APPLITE_SOURCES))))
RLS_VIDEO_DEMO_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(VIDEO_DEMO_SOURCES))))
RLS_STUB_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(STUB_SOURCES))))
RLS_TEST_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(TEST_SOURCES))))
RLS_PERF_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(PERF_SOURCES))))
RLS_RECORDER_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(RECORDER_SOURCES))))
RLS_RECORDING_SDK_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(RECORDING_SDK_SOURCES))))
RLS_PLAYER_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(PLAYER_SOURCES))))
RLS_JNI_OBJECTS = $(addprefix $(RLS_OBJ_PATH)/, $(addsuffix .o, $(basename $(JNI_SOURCES))))

.PHONY: all clean debug release pre voip sdk_lib sdk voip pstn

all: pre debug release

-include $(addprefix $(DBG_DEP_PATH)/, $(addsuffix .d, $(SOURCES)))
-include $(addprefix $(RLS_DEP_PATH)/, $(addsuffix .d, $(SOURCES)))

$(DBG_OBJ_PATH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DBG_DEP_PATH)/$<.d)
	$(CXX) -Wp,-MP,-MD,$(DBG_DEP_PATH)/$<.d -c $(DEBUG_CXXFLAGS) $(INCPATH) -o $@ $<
	@sed -e "s/\(.*\.o:\)/$(subst /,\/,$(dir $(DBG_OBJ_PATH)/$<))\1/" $(DBG_DEP_PATH)/$<.d >$(DBG_DEP_PATH)/$<.d.tmp
	@mv $(DBG_DEP_PATH)/$<.d.tmp $(DBG_DEP_PATH)/$<.d

$(DBG_OBJ_PATH)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DBG_DEP_PATH)/$<.d)
	$(CC) -Wp,-MP,-MD,$(DBG_DEP_PATH)/$<.d -c $(DEBUG_CFLAGS) $(INCPATH) -o "$@" "$<"
	@sed -e "s/\(.*\.o:\)/$(subst /,\/,$(dir $(DBG_OBJ_PATH)/$<))\1/" $(DBG_DEP_PATH)/$<.d >$(DBG_DEP_PATH)/$<.d.tmp
	@mv $(DBG_DEP_PATH)/$<.d.tmp $(DBG_DEP_PATH)/$<.d

$(RLS_OBJ_PATH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(RLS_DEP_PATH)/$<.d)
	$(CXX) -Wp,-MP,-MD,$(RLS_DEP_PATH)/$<.d -c $(RELEASE_CXXFLAGS) $(INCPATH) -o "$@" "$<"
	@sed -e "s/\(.*\.o:\)/$(subst /,\/,$(dir $(RLS_OBJ_PATH)/$<))\1/" $(RLS_DEP_PATH)/$<.d >$(RLS_DEP_PATH)/$<.d.tmp
	@mv $(RLS_DEP_PATH)/$<.d.tmp $(RLS_DEP_PATH)/$<.d

$(RLS_OBJ_PATH)/%.o: %.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(RLS_DEP_PATH)/$<.d)
	$(CC) -Wp,-MP,-MD,$(RLS_DEP_PATH)/$<.d -c $(RELEASE_CFLAGS) $(INCPATH) -o "$@" "$<"
	@sed -e "s/\(.*\.o:\)/$(subst /,\/,$(dir $(RLS_OBJ_PATH)/$<))\1/" $(RLS_DEP_PATH)/$<.d >$(RLS_DEP_PATH)/$<.d.tmp
	@mv $(RLS_DEP_PATH)/$<.d.tmp $(RLS_DEP_PATH)/$<.d

DBG_BASE_TARGET = $(DBG_BIN_PATH)/libbase.a
DBG_STUB_TARGET = $(DBG_BIN_PATH)/libstub.a
DBG_APPLITE_TARGET = $(DBG_BIN_PATH)/applite
DBG_VIDEO_DEMO_TARGET = $(DBG_BIN_PATH)/video_recorder_demo
DBG_PSTN_TARGET = $(DBG_BIN_PATH)/pseudo_stub
DBG_RECORDER_TARGET  = $(DBG_BIN_PATH)/agora_recorder
DBG_RECORDING_SDK_TARGET = $(DBG_BIN_PATH)/librecording.a
DBG_PLAYER_TARGET  = $(DBG_BIN_PATH)/agora_player

RLS_BASE_TARGET = $(RLS_BIN_PATH)/libbase.a
RLS_STUB_TARGET = $(RLS_BIN_PATH)/libstub.a
RLS_STUB_BIN_TARGET = $(RLS_BIN_PATH)/libstub.so
RLS_APPLITE_TARGET = $(RLS_BIN_PATH)/applite
RLS_PSTN_TARGET = $(RLS_BIN_PATH)/pseudo_stub
RLS_PERF_TARGET = $(RLS_BIN_PATH)/stress_test
RLS_RECORDING_SDK_TARGET = $(RLS_BIN_PATH)/librecording.a
RLS_RECORDER_TARGET  = $(RLS_BIN_PATH)/agora_recorder
RLS_PLAYER_TARGET  = $(RLS_BIN_PATH)/agora_player
RLS_JNI_TARGET = $(RLS_BIN_PATH)/libagora-rtc-server-jni.so

$(DBG_BASE_TARGET): $(DBG_BASE_OBJECTS)
	@mkdir -p $(dir $@)
	ar cqs "$@" $^

$(DBG_STUB_TARGET): $(DBG_STUB_OBJECTS)
	@mkdir -p $(dir $@)
	ar cqs "$@" $^

$(DBG_APPLITE_TARGET): $(DBG_APPLITE_OBJECTS) $(DBG_BASE_TARGET) $(DBG_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(DBG_APPLITE_OBJECTS) \
		-Llibs -L$(DBG_BIN_PATH) -lbase -lagorartc -lHDACEngine -lmp3lame

$(DBG_VIDEO_DEMO_TARGET): $(DBG_VIDEO_DEMO_OBJECTS) $(DBG_BASE_TARGET) $(DBG_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(DBG_VIDEO_DEMO_OBJECTS) \
		-Llibs/video -L$(DBG_BIN_PATH) -lbase -lagorartc -lHDACEngine

$(DBG_PSTN_TARGET): $(DBG_TEST_OBJECTS) $(DBG_BASE_TARGET) $(DBG_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(DBG_TEST_OBJECTS) -Llibs -L$(DBG_BIN_PATH) -lstub -lbase -lpthread

$(DBG_RECORDER_TARGET): $(DBG_RECORDER_OBJECTS) $(DBG_BASE_TARGET) $(DBG_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(DBG_RECORDER_OBJECTS) -Llibs -L$(DBG_BIN_PATH) \
		-lbase -lagorartc

$(DBG_RECORDING_SDK_TARGET): $(DBG_RECORDING_SDK_OBJECTS) $(DBG_BASE_OBJECTS)
	@mkdir -p $(dir $@)
	ar cqs "$@" $^

$(DBG_PLAYER_TARGET): $(DBG_PLAYER_OBJECTS) $(DBG_BASE_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(DBG_PLAYER_OBJECTS) -Llibs -L$(DBG_BIN_PATH) -lbase -lHDACEngine

$(RLS_BASE_TARGET): $(RLS_BASE_OBJECTS)
	@mkdir -p $(dir $@)
	ar cqs "$@" $^

$(RLS_STUB_TARGET): $(RLS_STUB_OBJECTS)
	@mkdir -p $(dir $@)
	ar cqs "$@" $^

$(RLS_STUB_BIN_TARGET): $(RLS_STUB_OBJECTS) $(RLS_BASE_TARGET)
	$(LINK) $(LDFLAGS) -shared -o "$@" $(RLS_STUB_OBJECTS) -L$(RLS_BIN_PATH) -lbase -Wl,-z,defs

AUDIO_SDK = HDACEngine
$(RLS_APPLITE_TARGET): $(RLS_APPLITE_OBJECTS) $(RLS_BASE_TARGET) $(RLS_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@"  $(RLS_APPLITE_OBJECTS) \
		-Llibs -L$(RLS_BIN_PATH) -lbase -lagorartc -l$(AUDIO_SDK) -lmp3lame -g

$(RLS_PSTN_TARGET): $(RLS_TEST_OBJECTS) $(RLS_BASE_TARGET) $(RLS_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(RLS_TEST_OBJECTS) -Llibs -L$(RLS_BIN_PATH) -lstub -lbase -lpthread

$(RLS_PERF_TARGET): $(RLS_PERF_OBJECTS) $(RLS_BASE_TARGET) $(RLS_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(RLS_PERF_OBJECTS) -Llibs -L$(RLS_BIN_PATH) -lstub -lbase -lpthread

$(RLS_RECORDER_TARGET): $(RLS_RECORDER_OBJECTS) $(RLS_BASE_TARGET) $(RLS_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(RLS_RECORDER_OBJECTS) -Llibs -L$(RLS_BIN_PATH) \
		-lbase -lagorartc

$(RLS_RECORDING_SDK_TARGET): $(RLS_RECORDING_SDK_OBJECTS) $(RLS_BASE_OBJECTS)
	@mkdir -p $(dir $@)
	ar cqs "$@" $^

$(RLS_PLAYER_TARGET): $(RLS_PLAYER_OBJECTS) $(RLS_BASE_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) $(LDFLAGS) -o "$@" $(RLS_PLAYER_OBJECTS) -Llibs -L$(RLS_BIN_PATH) -lbase -lHDACEngine

$(RLS_JNI_TARGET): $(RLS_JNI_OBJECTS) $(RLS_BASE_TARGET) $(RLS_STUB_TARGET)
	@mkdir -p $(dir $@)
	$(LINK) -shared $(LDFLAGS) -o "$@" -L$(RLS_BIN_PATH) $(RLS_JNI_OBJECTS) -lstub -lbase -Wl,-z,defs

debug: $(DBG_BASE_TARGET) $(DBG_STUB_TARGET) $(DBG_PSTN_TARGET) \
		$(DBG_APPLITE_TARGET) $(DBG_RECORDER_TARGET) \
		$(DBG_RECORDING_SDK_TARGET) $(DBG_PLAYER_TARGET)
		#$(DBG_VIDEO_DEMO_TARGET)
	ln -f libs/libHDACEngine.so $(DBG_BIN_PATH)/libHDACEngine.so
	ln -f libs/libagorartc.so $(DBG_BIN_PATH)/libagorartc.so

release: $(RLS_BASE_TARGET) $(RLS_STUB_TARGET) $(RLS_PSTN_TARGET) \
		$(RLS_APPLITE_TARGET) $(RLS_PERF_TARGET) $(RLS_RECORDER_TARGET) \
		$(RLS_RECORDING_SDK_TARGET) $(RLS_PLAYER_TARGET)
	ln -f libs/libHDACEngine.so $(RLS_BIN_PATH)/libHDACEngine.so
	ln -f libs/libagorartc.so $(RLS_BIN_PATH)/libagorartc.so

pre:
	@mkdir -p $(DBG_OBJ_PATH)
	@mkdir -p $(DBG_DEP_PATH)
	@mkdir -p $(DBG_BIN_PATH)
	@mkdir -p $(RLS_OBJ_PATH)
	@mkdir -p $(RLS_DEP_PATH)
	@mkdir -p $(RLS_BIN_PATH)

clean:
	rm -rf $(DBG_OBJ_PATH)
	rm -rf $(DBG_BIN_PATH)
	rm -rf $(RLS_OBJ_PATH)
	rm -rf $(RLS_BIN_PATH)
	rm -rf sdk
	rm -rf java_ext
	rm -rf recording
	rm -rf *.gz

sdk_lib: release
	@rm -rf $(RLS_BIN_PATH)/tmp
	@mkdir -p $(RLS_BIN_PATH)/tmp
	@cp $(RLS_BASE_TARGET) $(RLS_STUB_TARGET) $(RLS_BIN_PATH)/tmp/
	@cd $(RLS_BIN_PATH)/tmp && ar x libbase.a && ar x libstub.a && ar cqs libvoip.a *.o 

stub_bin: release $(RLS_STUB_BIN_TARGET)

pstn: release sdk_lib
	mkdir -p sdk/doc
	mkdir -p sdk/bin/
	mkdir -p sdk/libs/
	cp -r libs/*.so sdk/bin/
	cp doc/README.txt sdk/doc
	cp -r include sdk/
	rm -f sdk/include/recording_sdk.h
	cp -r $(TEST_SOURCES) sdk/
	cp -r $(RLS_APPLITE_TARGET) sdk/bin/
	mv $(RLS_BIN_PATH)/tmp/libvoip.a sdk/libs/
	cp -r data/ sdk
	tar cvzf voip-${VERSION}.tar.gz sdk

java: release jni
	@rm -rf java_ext
	@mkdir java_ext
	cd agora-rtc-server-java && gradle build && cp build/libs/*.jar ../java_ext/
	cp agora-rtc-server-java/ReadMe.txt java_ext/
	cp $(RLS_APPLITE_TARGET) java_ext
	cp libs/lib*.so java_ext
	cp libs/libH*.so java_ext
	cp $(RLS_JNI_TARGET) java_ext
	cp $(TEST_JAVA_SOURCES) java_ext
	cp -r data java_ext
	tar cvzf agora-rtc-server-java-${VERSION}.tar.gz java_ext/*

jni: $(RLS_JNI_TARGET)

recording: release
	@mkdir -p recording/bin/
	@mkdir -p recording/libs/
	@mkdir -p recording/doc/
	@cp -r libs/*.so recording/bin/
	@cp -r include recording/
	@cp internal/recap_format.h recording/include/
	@rm -f recording/include/agora_sdk_i.h
	@cp -r doc/Recorder.txt recording/doc/
	@cp -r $(RLS_RECORDER_TARGET) recording/bin/
	@cp -r $(RLS_PLAYER_TARGET) recording/bin/
	@cp test/recorder/demo.cpp recording/
	@cp $(RLS_RECORDING_SDK_TARGET) recording/libs/
	@tar cvzf recording-${VERSION}.tar.gz recording

