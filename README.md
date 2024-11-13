---

# DecoderSample

Простое консольное приложение для декодирования изображений в формате JPEG XS и сохранения их в формате PNG, используя библиотеку Open Visual Cloud's SVT-JPEG-XS.

## Использование

Запустите приложение из командной строки следующим образом:

```bash
DecoderSample.exe <file_name.jxs>
```

где `<file_name.jxs>` — это путь к вашему изображению в формате JPEG XS, которое вы хотите декодировать.

## Сборка

1. Обычный запуск CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```

## Ссылки

- **Проект SVT-JPEG-XS на GitHub**: [OpenVisualCloud/SVT-JPEG-XS](https://github.com/OpenVisualCloud/SVT-JPEG-XS)
- **Лицензия SVT-JPEG-XS**: [LICENSE](https://github.com/OpenVisualCloud/SVT-JPEG-XS/blob/main/LICENSE.md)

## Лицензия

Этот проект использует библиотеку SVT-JPEG-XS, которая распространяется под лицензией BSD-2-Clause. Полные условия лицензии доступны по ссылке: [LICENSE](https://github.com/OpenVisualCloud/SVT-JPEG-XS/blob/main/LICENSE.md).

