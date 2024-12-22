### Формат сообщений

#### Авторизация

1. **Запрос на авторизацию** (от клиента к серверу через `auth`):
   ```json
   {
       "action": "login",
       "username": "<username>",
       "password": "<password>"
   }
   ```
2. **Ответ сервера**:
   - Успех:
     ```json
     {
         "status": "success",
         "token": "<token>",
         "connection_name": "<connection name>"
     }
     ```
   - Ошибка:
     ```json
     {
         "status": "error",
         "message": "<error_message>"
     }
     ```

#### Отправка сообщения пользователю

1. **Запрос на отправку** (от клиента через `chat`):
   ```json
   {
       "action": "message",
       "token": "<token>",
       "to_user": "<username>",
       "message": "<message>"
   }
   ```

#### Создание группы

1. **Запрос на создание группы** (от клиента через `group`):
   ```json
   {
       "action": "create_group",
       "token": "<token>",
       "group_name": "<group_name>"
   }
   ```

#### Вступление в группу

1. **Запрос на вступление** (от клиента через `group`):
   ```json
   {
       "action": "join_group",
       "token": "<token>",
       "group_name": "<group_name>"
   }
   ```

#### Отправка сообщения в группу

1. **Запрос на отправку** (от клиента через `group`):
   ```json
   {
       "action": "message",
       "token": "<token>",
       "group_name": "<group_name>",
       "message": "<message>"
   }
   ```

#### Получение сообщений

Сервер посылает новые сообщения через соответствующее соединение:
```json
{
    "from": "<sender>",
    "type": "chat|group",
    "message": "<message>",
    "group_name": "<group_name>"  // Присутствует только если сообщение из группы
}
```

