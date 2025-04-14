add_executable(dserver dserver.cpp)
target_link_libraries(dserver  drpc) # Replace 'drpc' with your actual library if needed
install(TARGETS dserver DESTINATION bin)