# Telnet(like)

### Формулировка задачи
Реализовать сервер, запускающий оболочку (из переменной окружения SHELL) для каждого входящего подключения и передающий данные из её стандартных файлов в сокет и наоборот. Не делать предположений о диалоговом режиме, реализовывать полнодуплексную связь до разрыва соединения/завершения программы.
  
Опциональные доработки по желанию:  
 * (Только UNIX) открывать на сервере псевдотерминал, и запускать оболочку в нём, чтобы работало редактировнаие командной строки
 * Сделать поддержку кодирования 8-битных символов и поддержку опций Telnet

### Используемые технологии

* [io_uring](https://lwn.net/Articles/776703/) (linux kernel ≥ 5.12)
* [Boost.ASIO](https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio.html) ≥ 1.75.0
* [С++ coroutines](https://en.cppreference.com/w/cpp/language/coroutines) (≥ C++20)

