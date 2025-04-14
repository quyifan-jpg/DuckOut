add_executable(dclient dclient.cpp)
target_link_libraries(dclient drpc) # Replace 'drpc' with your actual library if needed
install(TARGETS dclient DESTINATION bin)