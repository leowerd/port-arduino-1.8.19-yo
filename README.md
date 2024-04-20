Это сборка Arduino IDE 1.8, не требующая установки. Содержит в себе все нужные библиотеки для компиляции прошивки yo-radio и плагина bender.
Порядок следующий:
1) Скачиваете ZIP, распаковываете в корень диска, чтобы избежать проблем с длинными путями и символами в именах
 
![изображение](https://github.com/leowerd/port-arduino-1.8.19-yo/assets/22613147/17f8820b-1368-4ee6-9358-c95d04aa8289)

2) Запускаете arduino.exe, открываете в нем скетч \yoradio-main\yoRadio\yoRadio.ino
  
3) Выбираете свою плату и порт в меню инструменты. Ключевой момент, убедитесь что разметка выбрана верно
 
![изображение](https://github.com/leowerd/port-arduino-1.8.19-yo/assets/22613147/6c36152f-1a55-4661-a0c0-2d1cd6c3c9a5)

4) Подключаете плату в режиме прошивки и выбираете пункт меню ESP32 Sketch Data Upload, подтверждаете выбраный каталог в окне.
   
![изображение](https://github.com/leowerd/port-arduino-1.8.19-yo/assets/22613147/a91bfb10-6019-4df0-80da-998ef1da3c9e)

5) Убедились, что процесс окончен, плата в режиме прошивки - запускайте компиляцию с загрузкой
   
![изображение](https://github.com/leowerd/port-arduino-1.8.19-yo/assets/22613147/f801140a-2cde-4a26-89c4-43f0add3f16f)

6) Дождались окончания процесса, плата ожидает подключения, подключаемся к точке доступа yo-radio (проще со смартфона), и вводдим данные своего роутера (имя\пароль).

7) Теперь радио доступно по IP адресу. Вы великолепны.

8) Последующие обновления плагина от Alexey244 кидайте в каталог \yoradio-main\yoRadio\ с заменой файла.

Ссылки на авторов:

проект Алекса https://alexgyver.ru/benderadio/

топик на форуме https://community.alexgyver.ru/threads/internet-radio-v-forme-golovy-bendera-obsuzhdenie-proekta.9158/

yoradio https://github.com/e2002/yoradio

автор плагина bender для yoradio https://community.alexgyver.ru/search/254375/
