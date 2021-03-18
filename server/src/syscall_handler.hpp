#ifndef SYSCALLS_HPP
#define SYSCALLS_HPP
#include <string>
#include <functional>

#include <errno.h>


namespace hse {

// This class was created to enable syscall handling and throwing system_errors on error syscall return
// The code:
//
// * void * ret = mmap(agr1, arg2, ...);
// * if(ret != -1)
// *     throw std::system_error(errno, "Description")
//
//
// Can be rewritten as
//
// * void * ret = try_syscall("Description", -1, mmap, arg1, arg2, ...);
//
// or like
//
// * void* ret = syscall_handler(mmap) (arg1, arg2, ...);
//
// if you don't care about return condition

//template<class T, class R, class... Args >
//R try_syscall(std::string error_description, T error_code, R func(Args...), Args...args)
//{
//    R answ = func(static_cast<Args>(args)...);
//    if(answ == static_cast<R>(error_code))
//        throw std::system_error(errno, error_description);
//    return answ;
//}


// This class was created to enable syscall handling and throwing system_errors on error syscall return
// The code:
//
// * void * ret = mmap(agr1, arg2, ...);
// * if(ret != -1)
// *     throw std::system_error(errno, "Description of error condition")
//
//
// Can be rewritten as
//
// * void * ret = syscall_handler(mmap)
// *              .set_description("Description of error condition")
// *              .set_error_return(-1)
// *              .run(arg1, arg2, ...);
//
// or like
//
// * void* ret = syscall_handler(mmap) (arg1, arg2, ...);
//
// if you don't care about return condition


template<class R, class... Args >
class syscall_handler{
private:
    std::string error_description="Error";
    bool has_error_return = false;
    std::function<R (Args...)> syscall;
    std::optional<std::function<R()>> error_condition = std::nullopt;


public:
    syscall_handler(R sys(Args...)): syscall(sys){}

    syscall_handler& set_description(std::string desc) {
        error_description = std::move(desc);
        return *this;
    }

    template<typename T>
    syscall_handler& set_error_return_value(T err_val) {
        has_error_return = true;
        error_condition  = [err_val]{ return err_val; };
        return *this;
    }

    R run(Args... args){
        R ret = this->syscall(args...);
        if(has_error_return){
            if((*error_condition)() == ret){
                throw std::system_error({errno, std::system_category()}, error_description);
            }
        }
        return ret;
    }

    R operator ()(Args... args){
        return this->run(args...);
    }

};

}

#endif // SYSCALLS_HPP
