#ifndef SYSCALLS_HPP
#define SYSCALLS_HPP
#include <string>
#include <functional>
#include <cerrno>
#include <system_error>
#include <variant>

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
// *              .set_error_condition(-1)
// *              .run(arg1, arg2, ...);
//
// or like
//
// * void* ret = syscall_handler(mmap) (arg1, arg2, ...);
//
// if you don't care about return condition


template<class R, class... Args >
class syscall_handler{
public:
    enum SH_ERROR_CONDITION{
        SINGLE_VALUE,
        PREDICATE,
        NO_CONITION
    };

private:
    using err_pred_t = std::function<bool(R)>;
    std::string error_description="Syscall error";
    SH_ERROR_CONDITION error_contion_type = NO_CONITION;
    std::function<R (Args...)> syscall;
    std::variant<R,
                 std::monostate,
                 std::function<bool(R)>>  error_condition;


    void check_error(R ret){
        switch (error_contion_type) {
            case SINGLE_VALUE :
                if(std::get<R>(error_condition) == ret)
                    throw std::system_error({errno, std::system_category()}, error_description);
                break;

            case PREDICATE :
                if(std::get<std::function<bool(R)>>(error_condition) (ret))
                    throw std::system_error({errno, std::system_category()}, error_description);
                break;

        }
    }
public:

    syscall_handler(R sys(Args...)): syscall(sys){}

    syscall_handler& set_description(std::string desc) {
        error_description = std::move(desc);
        return *this;
    }

    template<typename Ret>
    syscall_handler& set_error_condition(R err_val) {
        error_contion_type = SINGLE_VALUE;
        error_condition  = static_cast<R>(err_val);
        return *this;
    }

    syscall_handler& set_error_condition(std::function<bool(R)> err_predicate) {
        error_contion_type = PREDICATE;
        error_condition = err_predicate;
        return *this;
    }

    R run(Args... args){
        R ret = this->syscall(args...);

        check_error(ret);

        return ret;
    }

    R operator ()(Args... args){
        return this->run(args...);
    }

};

}

#endif // SYSCALLS_HPP
