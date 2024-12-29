#ifndef INCLUDED_app_context_hpp
#define INCLUDED_app_context_hpp

#include "io_context.hpp"
#include "thread.hpp"
#include "component.hpp"

namespace jm {


template <typename T>
concept DerivedFromComponent = std::is_base_of_v<Component, T>;


class AppContext : boost::noncopyable {
    IoContext ioContext_;
    std::vector<Thread> threads_;

    static void initialize();
    void waitForAppExit();
    void start();

  public:
    AppContext();
    ~AppContext();

    static AppContext &getContext();

    IoContext &getIoContext() { return ioContext_; }


    template <DerivedFromComponent T> static void run() {
        initialize();
        T::preConstruct();
        auto mainClass = std::make_unique<T>();
        mainClass->postConstruct();
        getContext().waitForAppExit();
        mainClass.reset();
    }
};

} // namespace jm

#endif // INCLUDED_app_context_hpp