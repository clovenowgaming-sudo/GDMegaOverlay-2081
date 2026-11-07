include(FetchContent)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")

FetchContent_Declare(
    nhjson
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
message("Fetching nlohmann-json")
FetchContent_MakeAvailable(nhjson)
