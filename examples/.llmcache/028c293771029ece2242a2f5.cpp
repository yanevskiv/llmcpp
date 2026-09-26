// llmcpp cache entry
// function: template <typename T> void f()
// location: main.cpp:3:23
// model: claude-opus-5-5
// date: 2026-09-26T18:07:14Z
// prompt:
//   Use std::cout to print what is T here.
// ---
const std::string pretty = __PRETTY_FUNCTION__;
const std::string key = "T = ";
const std::string::size_type start = pretty.find(key);
if (start == std::string::npos) {
    std::cout << pretty << '\n';
} else {
    const std::string::size_type begin = start + key.size();
    const std::string::size_type end = pretty.find_first_of(";]", begin);
    std::cout << pretty.substr(begin, end == std::string::npos ? std::string::npos : end - begin) << '\n';
}
