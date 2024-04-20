import asyncio
import logging
import voluptuous as vol
import json
import urllib.request

from homeassistant.components import media_source
from homeassistant.helpers.aiohttp_client import async_get_clientsession

_LOGGER      = logging.getLogger(__name__)

VERSION = '0.9.260'

DOMAIN = "yoradio"

from homeassistant.helpers import config_validation as cv

from homeassistant.components.media_player.browse_media import (
    async_process_play_media_url,
)
from homeassistant.components.media_player import (
  MediaPlayerEntity,
  MediaPlayerEnqueue,
  BrowseMedia,
  PLATFORM_SCHEMA
)

from homeassistant.components.media_player.const import (
  MEDIA_TYPE_MUSIC,
  MEDIA_TYPE_URL,
  SUPPORT_TURN_ON, SUPPORT_TURN_OFF,
  SUPPORT_VOLUME_STEP, SUPPORT_VOLUME_SET,
  SUPPORT_PAUSE, SUPPORT_PLAY, SUPPORT_STOP,
  SUPPORT_PREVIOUS_TRACK, SUPPORT_NEXT_TRACK,
  SUPPORT_SELECT_SOURCE, SUPPORT_BROWSE_MEDIA, SUPPORT_PLAY_MEDIA
)

from homeassistant.const import (
  CONF_NAME,
  STATE_IDLE,
  STATE_PLAYING,
  STATE_OFF,
  STATE_ON,
)

SUPPORT_YORADIO = SUPPORT_PAUSE | SUPPORT_PLAY | SUPPORT_STOP |\
                  SUPPORT_VOLUME_SET | SUPPORT_VOLUME_STEP | \
                  SUPPORT_TURN_ON | SUPPORT_TURN_OFF | \
                  SUPPORT_PREVIOUS_TRACK | SUPPORT_NEXT_TRACK | \
                  SUPPORT_SELECT_SOURCE | SUPPORT_BROWSE_MEDIA | SUPPORT_PLAY_MEDIA

DEFAULT_NAME = 'yoRadio'
CONF_MAX_VOLUME = 'max_volume'
CONF_ROOT_TOPIC = 'root_topic'

PLATFORM_SCHEMA = PLATFORM_SCHEMA.extend({
  vol.Required(CONF_ROOT_TOPIC, default="yoradio"): cv.string,
  vol.Optional(CONF_NAME, default=DEFAULT_NAME): cv.string,
  vol.Optional(CONF_MAX_VOLUME, default='254'): cv.string
})

def setup_platform(hass, config, add_devices, discovery_info=None):
  root_topic = config.get(CONF_ROOT_TOPIC)
  name = config.get(CONF_NAME)
  max_volume = int(config.get(CONF_MAX_VOLUME))
  session = async_get_clientsession(hass)
  playlist = []
  api = yoradioApi(root_topic, session, hass, playlist)
  add_devices([yoradioDevice(name, max_volume, api)], True)

class yoradioApi():
  def __init__(self, root_topic, session, hass, playlist):
    self.session = session
    self.hass = hass
    self.mqtt = hass.components.mqtt
    self.root_topic = root_topic
    self.playlist = playlist
    self.playlisturl = ""

  async def set_command(self, command):
    try:
      self.mqtt.async_publish(self.root_topic + '/command', command)
    except:
      await self.mqtt.async_publish(self.hass, self.root_topic + '/command', command)

  async def set_volume(self, volume):
    command = "vol " + str(volume)
    try:
      self.mqtt.async_publish(self.root_topic + '/command', command)
    except:
      await self.mqtt.async_publish(self.hass, self.root_topic + '/command', command)
      
  def fetch_data(self):
    try:
      html = urllib.request.urlopen(self.playlisturl).read().decode("utf-8")
      return str(html)
    except Exception as e:
      _LOGGER.error("Unable to fetch data: " + str(e))
      return ""
        
  async def set_source(self, source):
    number = source.split('.')
    command = "play " + number[0]
    try:
      self.mqtt.async_publish(self.root_topic + '/command', command)
    except:
      await self.mqtt.async_publish(self.hass, self.root_topic + '/command', command)

  async def set_browse_media(self, media_content_id):
    try:
      self.mqtt.async_publish(self.root_topic + '/command', media_content_id)
    except:
      await self.mqtt.async_publish(self.hass, self.root_topic + '/command', media_content_id)
      
  async def load_playlist(self, msg):
    try:
      self.playlisturl = msg.payload
      file = await self.hass.async_add_executor_job(self.fetch_data)
    except uException as e:
      _LOGGER.error('Error downloading ' + msg.payload)
    else:
      file = file.split('\n')
      counter = 1
      self.playlist.clear()
      for line in file:
        res = line.split('\t')
        if res[0] != "":
          station = str(counter) + '. ' + res[0]
          self.playlist.append(station)
          counter=counter+1

class yoradioDevice(MediaPlayerEntity):
  def __init__(self, name, max_volume, api):
    self._name = name
    self.api = api
    self._state = STATE_OFF
    self._current_source = None
    self._media_title = ''
    self._track_artist = ''
    self._track_album_name = ''
    self._volume = 0
    self._muted = False
    self._max_volume = max_volume
    self.api.mqtt.subscribe(self.api.root_topic+'/status', self.status_listener, 0, "utf-8")
    self.api.mqtt.subscribe(self.api.root_topic+'/playlist', self.playlist_listener, 0, "utf-8")
    self.api.mqtt.subscribe(self.api.root_topic+'/volume', self.volume_listener, 0, "utf-8")

  async def status_listener(self, msg):
    js = json.loads(msg.payload)
    self._media_title = js['title']
    self._track_artist = js['name']
    if js['on']==1:
      self._state = STATE_PLAYING if js['status']==1 else STATE_IDLE
    else:
      self._state = STATE_PLAYING if js['status']==1 else STATE_OFF
    self._current_source = str(js['station']) + '. ' + js['name']
    try:
      self.async_schedule_update_ha_state()
    except:
      pass

  async def playlist_listener(self, msg):
    await self.api.load_playlist(msg)
    try:
      self.async_schedule_update_ha_state()
    except:
      pass

  async def volume_listener(self, msg):
    self._volume = int(msg.payload) / self._max_volume
    try:
      self.async_schedule_update_ha_state()
    except:
      pass

  @property
  def supported_features(self):
    return SUPPORT_YORADIO

  @property
  def name(self):
    return self._name

  @property
  def media_title(self):
    return self._media_title

  @property
  def media_artist(self):
    return self._track_artist

  @property
  def media_album_name(self):
    return self._track_album_name

  @property
  def state(self):
    return self._state

  @property
  def volume_level(self):
    return self._volume

  async def async_set_volume_level(self, volume):
    await self.api.set_volume(round(volume * self._max_volume,1))

  @property
  def source(self):
    return self._current_source

  @property
  def source_list(self):
    return self.api.playlist

  async def async_browse_media(
    self, media_content_type: str | None = None, media_content_id: str | None = None
  ) -> BrowseMedia:
    #await self.api.set_browse_media(media_content_id)
    """Implement the websocket media browsing helper."""
    return await media_source.async_browse_media(
      self.hass,
      media_content_id,
    )
    
  async def async_play_media(
    self,
    media_type: str,
    media_id: str,
    enqueue: MediaPlayerEnqueue | None = None,
    announce: bool | None = None, **kwargs
  ) -> None:
    """Play a piece of media."""
    if media_source.is_media_source_id(media_id):
      media_type = MEDIA_TYPE_URL
      play_item = await media_source.async_resolve_media(self.hass, media_id, self.entity_id)
      media_id = async_process_play_media_url(self.hass, play_item.url)
    #if media_type in (MEDIA_TYPE_URL):
    #  media_id = async_process_play_media_url(self.hass, media_id)
    await self.api.set_browse_media(media_id)
    
  async def async_select_source(self, source):
    await self.api.set_source(source)
    self._current_source = source

  async def async_volume_up(self):
      newVol = float(self._volume) + 0.05
      await self.async_set_volume_level(newVol)
      self._volume = newVol

  async def async_volume_down(self):
      newVol = float(self._volume) - 0.05
      await self.async_set_volume_level(newVol)
      self._volume = newVol

  async def async_media_next_track(self):
      await self.api.set_command("next")

  async def async_media_previous_track(self):
      await self.api.set_command("prev")

  async def async_media_stop(self):
      await self.api.set_command("stop")
      self._state = STATE_IDLE

  async def async_turn_on(self):
      await self.api.set_command("start")
      self._state = STATE_PLAYING

  async def async_media_play(self):
      await self.api.set_command("start")
      self._state = STATE_PLAYING

  async def async_media_pause(self):
      await self.api.set_command("stop")
      self._state = STATE_IDLE
  
  async def async_turn_off(self):
      await self.api.set_command("turnoff")
      self._state = STATE_OFF

  async def async_turn_on(self, **kwargs):
      await self.api.set_command("turnon")
      self._state = STATE_ON
      
