#include "xtd/containers/Span.h"

namespace xtd::containers {
    using StringView = Span<const char>;
    using WStringView = Span<const wchar_t>;
}