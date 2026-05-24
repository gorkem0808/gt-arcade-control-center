import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

try:
    import serial
    from serial.tools import list_ports
except Exception as e:
    serial = None
    list_ports = None

APP_TITLE = "GT ARCADE CONTROL CENTER"

class SerialDevice:
    def __init__(self, port, hello):
        self.port = port
        self.hello = hello.strip()
        self.kind = "UNKNOWN"
        self.player = ""
        if "HELLO,MOUSE,P1" in self.hello:
            self.kind = "MOUSE"
            self.player = "P1"
        elif "HELLO,MOUSE,P2" in self.hello:
            self.kind = "MOUSE"
            self.player = "P2"
        elif "HELLO,KEYBOARD" in self.hello:
            self.kind = "KEYBOARD"
            self.player = "BUTTON"
        self.ser = None
        self.last_line = ""
        self.raw_x = None
        self.raw_y = None
        self.hid_x = None
        self.hid_y = None
        self.active = None
        self.filter_shift = None
        self.cal = None
        self.buttons = {}
        self.running = False

    def open(self):
        self.ser = serial.Serial(self.port, 115200, timeout=0.05)
        self.running = True
        threading.Thread(target=self.reader, daemon=True).start()

    def close(self):
        self.running = False
        try:
            if self.ser:
                self.ser.close()
        except Exception:
            pass

    def send(self, line):
        if not self.ser:
            return
        try:
            self.ser.write((line.strip() + "\n").encode("ascii", errors="ignore"))
        except Exception:
            pass

    def reader(self):
        while self.running:
            try:
                data = self.ser.readline().decode("ascii", errors="ignore").strip()
                if data:
                    self.parse_line(data)
            except Exception:
                time.sleep(0.1)

    def parse_line(self, line):
        self.last_line = line
        parts = line.split(',')
        if len(parts) < 2:
            return
        if parts[0] != "STATUS":
            return
        if len(parts) >= 3 and parts[1] == "MOUSE":
            # STATUS,MOUSE,P1,RAW,x,y,HID,x,y,ACTIVE,0,CAL,xmin,xmax,ymin,ymax,FILTER,2
            try:
                self.player = parts[2]
                i = 3
                while i < len(parts):
                    key = parts[i]
                    if key == "RAW":
                        self.raw_x = int(parts[i+1]); self.raw_y = int(parts[i+2]); i += 3
                    elif key == "HID":
                        self.hid_x = int(parts[i+1]); self.hid_y = int(parts[i+2]); i += 3
                    elif key == "ACTIVE":
                        self.active = bool(int(parts[i+1])); i += 2
                    elif key == "CAL":
                        self.cal = tuple(map(int, parts[i+1:i+5])); i += 5
                    elif key == "FILTER":
                        self.filter_shift = int(parts[i+1]); i += 2
                    else:
                        i += 1
            except Exception:
                pass
        elif len(parts) >= 3 and parts[1] == "KEYBOARD":
            self.buttons.clear()
            for item in parts[3:]:
                if ':' in item:
                    p, v = item.split(':', 1)
                    self.buttons[p] = (v == '1')

class ControlCenter(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(APP_TITLE)
        self.geometry("980x680")
        self.devices = {}
        self.captures = {"P1": {}, "P2": {}}
        self.create_ui()
        self.after(500, self.refresh_ui)

    def create_ui(self):
        top = ttk.Frame(self, padding=10)
        top.pack(fill="x")
        ttk.Label(top, text="GT ARCADE CONTROL CENTER", font=("Segoe UI", 16, "bold")).pack(side="left")
        ttk.Button(top, text="Cihazları Tara", command=self.scan_devices).pack(side="right")

        self.status = ttk.Label(self, text="Hazır. Önce 3 Pico'yu USB'ye tak ve 'Cihazları Tara' butonuna bas.", padding=8)
        self.status.pack(fill="x")

        self.tabs = ttk.Notebook(self)
        self.tabs.pack(fill="both", expand=True, padx=10, pady=10)

        self.dev_tab = ttk.Frame(self.tabs)
        self.p1_tab = ttk.Frame(self.tabs)
        self.p2_tab = ttk.Frame(self.tabs)
        self.key_tab = ttk.Frame(self.tabs)
        self.help_tab = ttk.Frame(self.tabs)
        self.tabs.add(self.dev_tab, text="Cihaz Durumu")
        self.tabs.add(self.p1_tab, text="Player 1 Kalibrasyon")
        self.tabs.add(self.p2_tab, text="Player 2 Kalibrasyon")
        self.tabs.add(self.key_tab, text="Tuş Testi")
        self.tabs.add(self.help_tab, text="Notlar")

        self.dev_text = tk.Text(self.dev_tab, height=20)
        self.dev_text.pack(fill="both", expand=True, padx=8, pady=8)

        self.p_vars = {}
        self.make_mouse_tab(self.p1_tab, "P1")
        self.make_mouse_tab(self.p2_tab, "P2")
        self.make_keyboard_tab()
        self.make_help_tab()

    def make_mouse_tab(self, parent, player):
        frame = ttk.Frame(parent, padding=12)
        frame.pack(fill="both", expand=True)
        ttk.Label(frame, text=f"{player} ABSOLUTE MOUSE KALİBRASYON", font=("Segoe UI", 14, "bold")).pack(anchor="w")
        info = ttk.Label(frame, text="4 köşe kalibrasyon: Sol Üst, Sağ Üst, Sağ Alt, Sol Alt. Orta nokta yok.")
        info.pack(anchor="w", pady=(0,10))

        vars = {
            "raw": tk.StringVar(value="RAW: -"),
            "hid": tk.StringVar(value="HID: -"),
            "active": tk.StringVar(value="AKTİF: -"),
            "cal": tk.StringVar(value="CAL: -"),
            "filter": tk.IntVar(value=2),
        }
        self.p_vars[player] = vars

        for key in ["raw", "hid", "active", "cal"]:
            ttk.Label(frame, textvariable=vars[key], font=("Consolas", 12)).pack(anchor="w")

        cap_frame = ttk.LabelFrame(frame, text="Köşe yakalama", padding=10)
        cap_frame.pack(fill="x", pady=12)
        for label, code in [("Sol Üst", "LU"), ("Sağ Üst", "RU"), ("Sağ Alt", "RD"), ("Sol Alt", "LD")]:
            ttk.Button(cap_frame, text=f"{label} köşeyi al", command=lambda c=code,p=player: self.capture_corner(p,c)).pack(side="left", padx=5)
        ttk.Button(cap_frame, text="Kalibrasyonu Pico'ya Kaydet", command=lambda p=player: self.save_calibration(p)).pack(side="left", padx=12)
        ttk.Button(cap_frame, text="Kalibrasyonu Sıfırla", command=lambda p=player: self.reset_cal(p)).pack(side="left", padx=5)

        filter_frame = ttk.LabelFrame(frame, text="Titreşim Engelleme", padding=10)
        filter_frame.pack(fill="x", pady=12)
        ttk.Label(filter_frame, text="0 hızlı / az filtre  -  6 çok sakin / fazla filtre").pack(anchor="w")
        scale = ttk.Scale(filter_frame, from_=0, to=6, orient="horizontal", command=lambda v,p=player: vars["filter"].set(int(float(v))))
        scale.set(2)
        scale.pack(fill="x", padx=5, pady=8)
        ttk.Button(filter_frame, text="Titreşim Ayarını Kaydet", command=lambda p=player: self.save_filter(p)).pack(anchor="w")

    def make_keyboard_tab(self):
        outer = ttk.Frame(self.key_tab, padding=12)
        outer.pack(fill="both", expand=True)
        ttk.Label(outer, text="GT ARCADE BUTTON KEYBOARD - TUŞ TESTİ", font=("Segoe UI", 14, "bold")).pack(anchor="w")
        self.key_list = tk.Text(outer, height=28, font=("Consolas", 12))
        self.key_list.pack(fill="both", expand=True, pady=10)

    def make_help_tab(self):
        text = tk.Text(self.help_tab, wrap="word", font=("Segoe UI", 11))
        text.pack(fill="both", expand=True, padx=8, pady=8)
        text.insert("end", """
Sistem yapısı:

Pico 1 = GT ARCADE P1 ABSOLUTE MOUSE
Pico 2 = GT ARCADE P2 ABSOLUTE MOUSE
Pico 3 = GT ARCADE BUTTON KEYBOARD

TeknoParrot tarafında cihazlar ayrı seçilecek:
Player 1 Light Gun -> GT ARCADE P1 ABSOLUTE MOUSE
Player 2 Light Gun -> GT ARCADE P2 ABSOLUTE MOUSE
Klavye tuşları -> GT ARCADE BUTTON KEYBOARD

Önemli:
- P1/P2 mouse firmware'inde klavye HID yoktur. Bu, TeknoParrot uyumluluğu için bilinçli yapıldı.
- GP20 basılı değilken mouse raporu gönderilmez; normal PC mouse serbest kalır.
- PC programı sadece ayar, test ve kalibrasyon içindir. Oyun sırasında kapatılabilir.
""")
        text.config(state="disabled")

    def scan_devices(self):
        if serial is None:
            messagebox.showerror("Eksik modül", "pyserial yüklü değil. pc_app klasöründeki BAT dosyasını çalıştırın.")
            return
        for d in list(self.devices.values()):
            d.close()
        self.devices.clear()
        ports = list(list_ports.comports())
        found = []
        for p in ports:
            try:
                ser = serial.Serial(p.device, 115200, timeout=0.2)
                time.sleep(0.2)
                ser.write(b"PING\n")
                time.sleep(0.2)
                hello = ""
                t0 = time.time()
                while time.time() - t0 < 0.8:
                    line = ser.readline().decode("ascii", errors="ignore").strip()
                    if line.startswith("HELLO"):
                        hello = line
                        break
                ser.close()
                if hello:
                    dev = SerialDevice(p.device, hello)
                    dev.open()
                    key = dev.player if dev.kind == "MOUSE" else "KEYBOARD"
                    self.devices[key] = dev
                    found.append(f"{key}: {p.device} - {hello}")
            except Exception:
                pass
        self.status.config(text="Bulunan cihazlar: " + (" | ".join(found) if found else "Yok"))

    def get_mouse(self, player):
        return self.devices.get(player)

    def capture_corner(self, player, corner):
        dev = self.get_mouse(player)
        if not dev or dev.raw_x is None:
            messagebox.showwarning("Cihaz yok", f"{player} cihazı bulunamadı veya veri yok.")
            return
        self.captures[player][corner] = (dev.raw_x, dev.raw_y)
        messagebox.showinfo("Kaydedildi", f"{player} {corner} = X:{dev.raw_x} Y:{dev.raw_y}")

    def save_calibration(self, player):
        dev = self.get_mouse(player)
        caps = self.captures[player]
        needed = ["LU", "RU", "RD", "LD"]
        if not dev:
            messagebox.showwarning("Cihaz yok", f"{player} cihazı bulunamadı.")
            return
        if any(k not in caps for k in needed):
            messagebox.showwarning("Eksik köşe", "4 köşenin tamamını yakala.")
            return
        xs = [caps[k][0] for k in needed]
        ys = [caps[k][1] for k in needed]
        x_min, x_max = min(xs), max(xs)
        y_min, y_max = min(ys), max(ys)
        dev.send(f"SETCAL,{x_min},{x_max},{y_min},{y_max}")
        messagebox.showinfo("Gönderildi", f"{player} kalibrasyon gönderildi.\nX:{x_min}-{x_max}\nY:{y_min}-{y_max}")

    def save_filter(self, player):
        dev = self.get_mouse(player)
        if not dev:
            messagebox.showwarning("Cihaz yok", f"{player} cihazı bulunamadı.")
            return
        val = self.p_vars[player]["filter"].get()
        dev.send(f"FILTER,{val}")
        messagebox.showinfo("Gönderildi", f"{player} titreşim filtresi: {val}")

    def reset_cal(self, player):
        dev = self.get_mouse(player)
        if dev:
            dev.send("RESETCAL")

    def refresh_ui(self):
        self.dev_text.delete("1.0", "end")
        for key, dev in self.devices.items():
            self.dev_text.insert("end", f"{key} | {dev.port} | {dev.hello}\nSon veri: {dev.last_line}\n\n")
            if dev.kind == "MOUSE" and dev.player in self.p_vars:
                v = self.p_vars[dev.player]
                v["raw"].set(f"RAW: X={dev.raw_x}  Y={dev.raw_y}")
                v["hid"].set(f"HID: X={dev.hid_x}  Y={dev.hid_y}")
                v["active"].set(f"AKTİF: {'EVET' if dev.active else 'HAYIR'}")
                v["cal"].set(f"CAL: {dev.cal}")
        kb = self.devices.get("KEYBOARD")
        self.key_list.delete("1.0", "end")
        if kb:
            order = ["2","3","4","5","6","7","8","17","18","19","9","10","11","12","13","14","15","16","21","22","28"]
            labels = {"2":"GP2=1","3":"GP3=2","4":"GP4=3","5":"GP5=4","6":"GP6=5","7":"GP7=6","8":"GP8=7","17":"GP17=8","18":"GP18=9","19":"GP19=0","9":"GP9=A","10":"GP10=B","11":"GP11=C","12":"GP12=D","13":"GP13=E","14":"GP14=F","15":"GP15=G","16":"GP16=H","21":"GP21=I","22":"GP22=J","28":"GP28=K"}
            for p in order:
                state = "BASILDI" if kb.buttons.get(p, False) else "BASILMADI"
                self.key_list.insert("end", f"{labels[p]:10s} : {state}\n")
        else:
            self.key_list.insert("end", "Klavye Pico bulunamadı. Cihazları Tara butonuna bas.\n")
        self.after(250, self.refresh_ui)

if __name__ == "__main__":
    app = ControlCenter()
    app.mainloop()
