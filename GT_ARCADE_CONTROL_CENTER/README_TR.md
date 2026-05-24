# GT ARCADE CONTROL CENTER

Bu proje 3 adet Raspberry Pi Pico ile çalışır:

1. **Pico 1** = Player 1 Absolute Mouse
2. **Pico 2** = Player 2 Absolute Mouse
3. **Pico 3** = Button Keyboard Board

Amaç: TeknoParrot gibi emülatörlerde potansiyometreli silah sistemini eski çalışan TinyUSB HID Absolute Mouse yapısını bozmadan kullanmak, tuşları ayrı Pico üzerinden klavye olarak vermek ve PC programı ile kalibrasyon/test yapmaktır.

## Çıkacak UF2 dosyaları

GitHub Actions sonunda şu dosyalar oluşur:

- `gt_arcade_p1_absolute_mouse.uf2`
- `gt_arcade_p2_absolute_mouse.uf2`
- `gt_arcade_button_keyboard.uf2`

## PC programı

`pc_app/GT_ARCADE_CONTROL_CENTER.bat` dosyasını çalıştır.

Program ile:

- P1 kalibrasyon
- P2 kalibrasyon
- Titreşim engelleme
- Tuş testi
- Cihaz testi

yapılır.

## Önemli karar

Mouse Pico'larında klavye HID yoktur. Bu bilinçli yapıldı. Böylece TeknoParrot eski çalışan absolute mouse yapısını daha güvenli algılar.
