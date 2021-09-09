#pragma once

#include <zeno/utils/api.h>
#include <zeno/utils/source_location.h>
#include <zeno/utils/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/core.h>
#include <string_view>
#include <sstream>
#include <memory>

namespace zeno {

ZENO_API spdlog::logger *get_spdlog_logger();

namespace __log_print {
    struct format_string {
        std::string_view str;
        source_location loc;

        template <class Str>
        format_string(Str str, source_location loc = source_location::current())
            : str(str), loc(loc) {}

        operator auto const &() const { return str; }
    };
};

template <class ...Args>
static void log_print(spdlog::level::level_enum log_level, __log_print::format_string const &fmt, Args &&...args) {
    spdlog::source_loc loc(fmt.loc.file_name(), fmt.loc.line(), fmt.loc.function_name());
    get_spdlog_logger()->log(loc, log_level, fmt::format(fmt.str, std::forward<Args>(args)...));
}

#define _PER_LOG_LEVEL(x, y) \
template <class ...Args> \
void log_##x(__log_print::format_string const &fmt, Args &&...args) { \
    log_print(spdlog::level::y, fmt, std::forward<Args>(args)...); \
}
_PER_LOG_LEVEL(trace, trace)
_PER_LOG_LEVEL(debug, debug)
_PER_LOG_LEVEL(info, info)
_PER_LOG_LEVEL(critical, critical)
_PER_LOG_LEVEL(warn, warn)
_PER_LOG_LEVEL(error, err)
#undef _PER_LOG_LEVEL

namespace loggerstd {

static inline constexpr struct {} endl;

static inline struct __logger_ostream {
    static thread_local std::stringstream ss;

    template <class T>
    __logger_ostream &operator<<(T const &x) {
        if constexpr (std::is_same_v<std::decay_t<decltype(endl)>, T>) {
            log_trace(ss.str());
            ss.clear();
        } else {
            ss << x;
        }
        return *this;
    }
} cout, cerr;

template <class ...Ts>
void printf(const char *fmt, Ts &&...ts) {
    log_trace(format(fmt, std::forward<Ts>(ts)...));
}

}

}
