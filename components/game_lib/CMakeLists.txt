idf_component_register(
  SRCS
    src/display.c
    src/joystick.c
    src/mqtt_game.c
    src/game.c
	src/cJSON.c

  INCLUDE_DIRS
    include

  REQUIRES
    mqtt
	esp_event
	driver
	esp_adc
)

