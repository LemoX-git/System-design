workspace "Домашнее задание 01 - Вариант 20" "Документирование архитектуры системы управления медицинскими записями" {

    model {
        admin = person "Администратор" "Создаёт пользователей системы и ищет их."
        registrar = person "Регистратор" "Регистрирует пациентов и ищет пациентов по ФИО."
        doctor = person "Врач" "Создаёт медицинские записи, привязывает их к пациенту и просматривает историю."

        idp = softwareSystem "Corporate Identity Provider" "Внешняя система аутентификации и управления корпоративными учётными записями." {
            tags "External"
        }

        notificationService = softwareSystem "Notification Service" "Внешний сервис служебных уведомлений." {
            tags "External"
        }

        medRecordsSystem = softwareSystem "Система управления медицинскими записями" "Система для регистрации пациентов и ведения медицинских записей." {
            webApp = container "Web Application" "Единый веб-интерфейс для администратора, регистратора и врача." "React SPA"
            bff = container "API Gateway / BFF" "Единая точка входа для клиентского приложения, маршрутизация запросов и оркестрация вызовов сервисов." "Spring Boot REST API"
            userService = container "User Management Service" "Создание пользователей, поиск по логину, поиск по маске имени и фамилии." "Spring Boot"
            patientRegistryService = container "Patient Registry Service" "Регистрация пациентов, поиск по ФИО, хранение карточек пациентов и истории записей." "Spring Boot"
            medicalRecordService = container "Medical Record Service" "Создание медицинских записей, поиск и получение записи по коду." "Spring Boot"

            userDb = container "User Database" "Хранение пользователей, ролей и служебных атрибутов." "PostgreSQL" {
                tags "Database"
            }

            patientDb = container "Patient Database" "Хранение пациентов и связей пациент-запись." "PostgreSQL" {
                tags "Database"
            }

            recordDb = container "Medical Record Database" "Хранение медицинских записей." "PostgreSQL" {
                tags "Database"
            }
        }

        admin -> medRecordsSystem "Создаёт пользователей и ищет их" "HTTPS"
        registrar -> medRecordsSystem "Регистрирует пациентов и ищет их" "HTTPS"
        doctor -> medRecordsSystem "Создаёт записи и просматривает историю пациента" "HTTPS"

        medRecordsSystem -> idp "Использует для аутентификации пользователей" "OpenID Connect / OAuth 2.0"
        medRecordsSystem -> notificationService "Отправляет служебные уведомления" "HTTPS / REST"

        admin -> webApp "Работает через веб-интерфейс" "HTTPS"
        registrar -> webApp "Работает через веб-интерфейс" "HTTPS"
        doctor -> webApp "Работает через веб-интерфейс" "HTTPS"

        webApp -> idp "Перенаправляет на аутентификацию" "OpenID Connect / OAuth 2.0"
        webApp -> bff "Вызывает бизнес-операции" "HTTPS / JSON"

        bff -> userService "Операции с пользователями" "HTTPS / REST"
        bff -> patientRegistryService "Операции с пациентами" "HTTPS / REST"
        bff -> medicalRecordService "Операции с медзаписями" "HTTPS / REST"

        userService -> idp "Провижининг учётных записей" "SCIM / REST"
        userService -> userDb "Читает и сохраняет пользователей" "JDBC / SQL"

        patientRegistryService -> patientDb "Читает и сохраняет карточки пациентов" "JDBC / SQL"
        patientRegistryService -> medicalRecordService "Проверяет существование записи" "HTTPS / REST"
        patientRegistryService -> notificationService "Отправляет служебные уведомления" "HTTPS / REST"

        medicalRecordService -> recordDb "Читает и сохраняет медицинские записи" "JDBC / SQL"
    }

    views {
        systemContext medRecordsSystem "SystemContext" {
            title "System Context - Система управления медицинскими записями"
            include *
            autoLayout lr
        }

        container medRecordsSystem "Containers" {
            title "Container - Система управления медицинскими записями"
            include *
            autoLayout lr
        }

        dynamic medRecordsSystem "CreateMedicalRecord" {
            title "Dynamic - Создание медицинской записи и добавление её пациенту"

            1: doctor -> webApp "Открывает карточку пациента и вводит данные новой записи"
            2: webApp -> bff "createMedicalRecord(patientId, payload)" "HTTPS / JSON"
            3: bff -> medicalRecordService "Создать запись" "HTTPS / REST"
            4: medicalRecordService -> recordDb "Сохранить запись" "JDBC / SQL"
            5: bff -> patientRegistryService "Добавить запись пациенту" "HTTPS / REST"
            6: patientRegistryService -> medicalRecordService "Проверить существование записи" "HTTPS / REST"
            7: patientRegistryService -> patientDb "Обновить историю пациента" "JDBC / SQL"
            8: patientRegistryService -> notificationService "Отправить уведомление" "HTTPS / REST"

            autoLayout lr 280 180
        }

        styles {
            element "Person" {
                shape Person
                background "#08427b"
                color "#ffffff"
                fontSize 28
            }

            element "Software System" {
                background "#1168bd"
                color "#ffffff"
                fontSize 26
            }

            element "Container" {
                background "#438dd5"
                color "#ffffff"
                fontSize 24
            }

            element "Database" {
                shape Cylinder
                background "#2e7d32"
                color "#ffffff"
                fontSize 24
            }

            element "External" {
                background "#999999"
                color "#ffffff"
                fontSize 24
            }

            relationship "Relationship" {
                fontSize 20
            }
        }

        theme default
    }
}