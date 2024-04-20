config.store.lastStation // последняя станция
config.station.name
config.station.title
config.store.volume // текущая громкость
config.store.balance
config.store.bass
config.store.middle
config.store.trebble
config.store.audioinfo
config.store.smartstart
config.store.tzHour
config.store.tzMin
config.store.brightness = (uint8_t)tzh;
WiFi.RSSI());
xPortGetFreeHeapSize());

player.prev(); // команда предыдучий
player.next(); // команда следующий
player.toggle(); // старт - стоп
player.sendCommand({PR_STOP, 0}); //стоп
player.sendCommand({PR_PLAY, config.store.lastStation}); //старт с последней станции
player.sendCommand({PR_PLAY, (uint16_t)sb});
player.stepVol(false); //тише на 1
player.stepVol(true);// громче на 1
player.setVol(volume); //установить громкость 0-254
config.save();
config.setTimezone((int8_t)tzh, 0);
config.setBrightness(true);
config.sleepForAfter(tzh, tzm);
config.setSmartStart(0);
ESP.restart();