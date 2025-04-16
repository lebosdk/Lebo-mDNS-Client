idf_component_register(SRCS "mdns_demo_main.cpp" "mdns/mdns_discovery.cc" "wifi/WiFiManager.cpp"
                    PRIV_REQUIRES spi_flash mdns esp_wifi nvs_flash
                    INCLUDE_DIRS "")

# 启用C++11支持
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_11)