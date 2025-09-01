CFLAGS := -std=c11 -g -ggdb -Wall

SANITIZE ?= 0

ifeq ($(SANITIZE),1)
	CFLAGS += -fsanitize=address
endif

CFLAGS += $(shell pkg-config --cflags libuv)
LFLAGS := $(shell pkg-config --libs   libuv)

OBJS := cJSON.o packet.o packet_handlers.o client.o data_types.o server.o packet_types.o log.o
OBJS := $(addprefix src/, $(OBJS))

DEPS := $(OBJS:%.o=%.d)

CXXFLAGS := $(CFLAGS) -std=c++20

GTEST_CFLAGS = $(shell pkg-config --cflags gtest_main)
GTEST_LIBS   = $(shell pkg-config --libs   gtest_main)

TEST_OBJS := buffers.o
TEST_OBJS := $(addprefix tests/, $(TEST_OBJS))

emcee: $(OBJS) src/main.c
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

emcee_tests: $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) $(LFLAGS) $(GTEST_LIBS) $^ -o $@

%.o: %.c %.h
	$(CC) -MMD -c $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) -MMD -c $(GTEST_CFLAGS) $(CXXFLAGS) -Isrc $< -o $@

-include $(DEPS)

clean:
	-rm -f emcee emcee_tests
	-rm -f $(OBJS) $(TEST_OBJS)
	-rm -f $(DEPS)
	-rm -rf *.dSYM

all: emcee emcee_tests

.PHONY: clean all
