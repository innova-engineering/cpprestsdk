/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 *
 * Simple Linux implementation of a static thread pool.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 ***/
#pragma once

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Winfinite-recursion"
#endif
#include "boost/asio.hpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__ANDROID__)
#include "pplx/pplx.h"
#include <atomic>
#include <jni.h>
#endif

#include "cpprest/details/cpprest_compat.h"

namespace crossplat
{
#if defined(__ANDROID__)
// IDEA: Break this section into a separate android/jni header
extern std::atomic<JavaVM*> JVM;
JNIEnv* get_jvm_env();

struct java_local_ref_deleter
{
    void operator()(jobject lref) const { crossplat::get_jvm_env()->DeleteLocalRef(lref); }
};

template<class T>
using java_local_ref = std::unique_ptr<typename std::remove_pointer<T>::type, java_local_ref_deleter>;
#endif

#if defined(_Win32)
template <int Major = 2, int Minor = 0>
class winsock_init_helper
{
public:
    winsock_init_helper()
    {

        if (InterlockedIncrement(&m_data.init_count) == 1)
        {
            WSADATA wsa_data;
            long result = WSAStartup(MAKEWORD(Major, Minor), &wsa_data);
            InterlockedExchange(&m_data.result, result);
        }
    }

    ~winsock_init_helper()
    {
        if (InterlockedDecrement(&m_data.init_count) == 0)
        {
            WSACleanup();
        }
    }

private:
    struct data
    {
        long init_count = 0;
        long result = 0;
    };

    static data m_data;
};
#endif

class threadpool
{
public:
    _ASYNCRTIMP static threadpool& shared_instance();
    _ASYNCRTIMP static std::unique_ptr<threadpool> __cdecl construct(size_t num_threads);

    virtual ~threadpool() = default;

    /// <summary>
    /// Initializes the cpprestsdk threadpool with a custom number of threads
    /// </summary>
    /// <remarks>
    /// This function allows an application (in their main function) to initialize the cpprestsdk
    /// threadpool with a custom threadcount. Libraries should avoid calling this function to avoid
    /// a diamond problem with multiple consumers attempting to customize the pool.
    /// </remarks>
    /// <exception cref="std::exception">Thrown if the threadpool has already been initialized</exception>
    static void initialize_with_threads(size_t num_threads);

    template<typename T>
    CASABLANCA_DEPRECATED("Use `.service().post(task)` directly.")
    void schedule(T task)
    {
        service().post(task);
    }

    boost::asio::io_service& service() { return m_service; }

protected:
    threadpool(size_t num_threads) : m_service(static_cast<int>(num_threads)) {}

#if defined(_Win32)
    winsock_init_helper<> m_init_helper;
#endif
    boost::asio::io_service m_service;
};

} // namespace crossplat
