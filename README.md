Пример работы с блоком Ethernet. Реализован для микроконтроллера Миландр 1986ВЕ1Т. www.milandr.ru

PC в Payload посылает два 16-ти разрядных параметра:
  - Длину ответного фрейма
  - Количество ответных фреймов
  
 В ответ микроконтроллер высылает заданное количество фреймов необходимого размера. Обмен происходит на уровне МАС адресов.

Является реализацией данной статьи - https://startmilandr.ru/doku.php/prog:ethernet:traffic_tx
