bin_PROGRAMS = test-linked-hash-map test-thread test-thread-pool test-logger test-libevent

test_linked_hash_map_SOURCES = test-linked-hash-map.cpp

test_linked_hash_map_CPPFLAGS = -I../src/utils

test_thread_CPPFLAGS = -I../src/utils

test_thread_LDFLAGS =

test_thread_SOURCES = test-thread.cpp ../src/utils/thread-job.cpp ../src/utils/thread.cpp

test_thread_pool_CPPFLAGS = -I../src/utils

test_thread_pool_LDFLAGS =

test_thread_pool_SOURCES = test-thread-pool.cpp ../src/utils/thread-job.cpp ../src/utils/thread.cpp ../src/utils/thread-pool.cpp

test_logger_SOURCES = test-logger.cpp

test_logger_CPPFLAGS = -I../src/utils

test_libevent_SOURCES = test-libevent.cpp ../src/utils/thread-job.cpp ../src/utils/thread.cpp ../src/utils/thread-pool.cpp ../src/utils/tcp-listener.cpp ../src/utils/tcp-server.cpp

test_libevent_CPPFLAGS = -I../src/utils

ACLOCAL_AMFLAGS = -I m4
