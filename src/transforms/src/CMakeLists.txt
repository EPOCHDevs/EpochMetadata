target_sources(epoch_metadata PRIVATE
        registration.cpp
        price_actions/smc/sessions_utils.cpp
)


add_subdirectory(cummulative)
add_subdirectory(futures)
add_subdirectory(hosseinmoein)
add_subdirectory(indicators)
add_subdirectory(operators)
add_subdirectory(price_actions)
add_subdirectory(tulip)
add_subdirectory(volatility)
add_subdirectory(statistics)
