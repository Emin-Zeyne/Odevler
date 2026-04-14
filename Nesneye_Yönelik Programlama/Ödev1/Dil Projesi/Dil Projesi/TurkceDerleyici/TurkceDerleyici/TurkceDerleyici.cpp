#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace std;

// ==========================================
// 1. انواع الرموز
// ==========================================
enum TokenType { ANAHTAR_KELIME, KIMLIK, SAYI, METIN_DEGER, ISARET, NOKTALAMA, BILINMEYEN };

struct Token {
    TokenType tur;
    string deger;
};

// ==========================================
// 2. كلاس التحليل اللغوي (Lexer)
// ==========================================
class Lexer {
private:
    string kaynakKod;
    int pozisyon;

    char suankiKarakter() {
        if (pozisyon >= kaynakKod.length()) return '\0';
        return kaynakKod[pozisyon];
    }
    void ilerle() { pozisyon++; }
    void boslukGec() { while (isspace(suankiKarakter())) ilerle(); }

public:
    Lexer(string kod) { kaynakKod = kod; pozisyon = 0; }

    vector<Token> tokenleriAyir() {
        vector<Token> tokenler;
        while (pozisyon < kaynakKod.length()) {
            boslukGec();
            char c = suankiKarakter();
            if (c == '\0') break;

            if (c == '"') {
                string metin = "";
                ilerle();
                while (suankiKarakter() != '"' && suankiKarakter() != '\0') {
                    metin += suankiKarakter();
                    ilerle();
                }
                ilerle();
                tokenler.push_back({ METIN_DEGER, metin });
            }
            else if (isdigit(c)) {
                string sayiStr = "";
                while (isdigit(suankiKarakter()) || suankiKarakter() == '.') {
                    sayiStr += suankiKarakter();
                    ilerle();
                }
                tokenler.push_back({ SAYI, sayiStr });
            }
            else if (isalpha(c)) {
                string kelime = "";
                while (isalnum(suankiKarakter())) {
                    kelime += suankiKarakter();
                    ilerle();
                }
                // أضفنا oku هنا ليتعرف عليها ككلمة محجوزة
                if (kelime == "tam" || kelime == "kesir" || kelime == "metin" || kelime == "yazdir" || kelime == "oku" || kelime == "eger" || kelime == "degilse" || kelime == "dongu") {
                    tokenler.push_back({ ANAHTAR_KELIME, kelime });
                }
                else {
                    tokenler.push_back({ KIMLIK, kelime });
                }
            }
            else if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '>' || c == '<' || c == '!') {
                string isaret(1, c);
                ilerle();
                if ((c == '=' || c == '!' || c == '>' || c == '<') && suankiKarakter() == '=') {
                    isaret += suankiKarakter();
                    ilerle();
                }
                tokenler.push_back({ ISARET, isaret });
                continue;
            }
            else if (c == ';' || c == '{' || c == '}' || c == '(' || c == ')') {
                string noktalama(1, c);
                tokenler.push_back({ NOKTALAMA, noktalama });
                ilerle();
            }
            else {
                string bilinmeyen(1, c);
                tokenler.push_back({ BILINMEYEN, bilinmeyen });
                ilerle();
            }
        }
        return tokenler;
    }
};

// ==========================================
// 3. كلاس المُفسّر والمنفذ (Yorumlayici)
// ==========================================
class Yorumlayici {
private:
    vector<Token> tokenler;
    int pozisyon;
    map<string, string> hafiza;
    bool sonEgerDurumu = false;
    vector<int> donguYigini;

    Token suankiToken() {
        if (pozisyon >= tokenler.size()) return { BILINMEYEN, "" };
        return tokenler[pozisyon];
    }
    void ilerle() { pozisyon++; }

    void hataVer(string mesaj) {
        cout << "\n[!] SINTAKS HATASI: " << mesaj << endl;
        exit(1);
    }

    Token bekle(TokenType beklenenTur, string hataMesaji) {
        Token t = suankiToken();
        if (t.tur != beklenenTur) hataVer(hataMesaji);
        ilerle();
        return t;
    }
    Token bekleDeger(TokenType tur, string deger, string hataMesaji) {
        Token t = suankiToken();
        if (t.tur != tur || t.deger != deger) hataVer(hataMesaji);
        ilerle();
        return t;
    }
    void blokAtla() {
        int parantez = 1;
        while (pozisyon < tokenler.size() && parantez > 0) {
            if (suankiToken().deger == "{") parantez++;
            else if (suankiToken().deger == "}") parantez--;
            ilerle();
        }
    }

    string sayiyiCevir(double d) {
        string str = to_string(d);
        str.erase(str.find_last_not_of('0') + 1, string::npos);
        if (str.back() == '.') str.pop_back();
        return str;
    }

    double sayiDegeriGetir(Token t) {
        if (t.tur == SAYI) return stod(t.deger);
        if (t.tur == KIMLIK) {
            if (hafiza.find(t.deger) != hafiza.end()) return stod(hafiza[t.deger]);
            else hataVer("Tanimlanmayan degisken: " + t.deger);
        }
        hataVer("Gecersiz sayisal deger!");
        return 0;
    }

    string ifadeHesapla() {
        Token t1 = suankiToken();
        ilerle();
        if (t1.tur == METIN_DEGER) return t1.deger;
        double sol = sayiDegeriGetir(t1);

        Token tIslem = suankiToken();
        if (tIslem.tur == ISARET && (tIslem.deger == "+" || tIslem.deger == "-" || tIslem.deger == "*" || tIslem.deger == "/")) {
            ilerle();
            Token t2 = suankiToken();
            ilerle();
            double sag = sayiDegeriGetir(t2);

            if (tIslem.deger == "+") return sayiyiCevir(sol + sag);
            if (tIslem.deger == "-") return sayiyiCevir(sol - sag);
            if (tIslem.deger == "*") return sayiyiCevir(sol * sag);
            if (tIslem.deger == "/") {
                if (sag == 0) hataVer("Sifira bolme hatasi! (Division by zero)");
                return sayiyiCevir(sol / sag);
            }
        }
        return sayiyiCevir(sol);
    }

    bool mantiksalHesapla() {
        double sol = sayiDegeriGetir(suankiToken());
        ilerle();
        Token isaret = bekle(ISARET, "Mantiksal isaret bekleniyor (>, <, >=, <=, ==, !=)");
        double sag = sayiDegeriGetir(suankiToken());
        ilerle();

        if (isaret.deger == ">") return sol > sag;
        if (isaret.deger == "<") return sol < sag;
        if (isaret.deger == ">=") return sol >= sag; // <-- تمت الإضافة!
        if (isaret.deger == "<=") return sol <= sag; // <-- تمت الإضافة!
        if (isaret.deger == "==") return sol == sag;
        if (isaret.deger == "!=") return sol != sag;

        hataVer("Bilinmeyen mantiksal isaret: " + isaret.deger);
        return false;
    }

public:
    Yorumlayici(vector<Token> t) { tokenler = t; pozisyon = 0; }

    void calistir() {
        while (pozisyon < tokenler.size()) {
            Token t = suankiToken();
            if (t.tur == BILINMEYEN && t.deger == "") break;

            if (t.tur == NOKTALAMA && t.deger == "}") {
                if (!donguYigini.empty()) {
                    pozisyon = donguYigini.back();
                    donguYigini.pop_back();
                }
                else {
                    ilerle();
                }
                continue;
            }

            if (t.tur == ANAHTAR_KELIME && t.deger == "eger") {
                ilerle(); bekleDeger(NOKTALAMA, "(", "'(' bekleniyor!");
                sonEgerDurumu = mantiksalHesapla();
                bekleDeger(NOKTALAMA, ")", "')' bekleniyor!"); bekleDeger(NOKTALAMA, "{", "'{' bekleniyor!");
                if (!sonEgerDurumu) blokAtla();
                continue;
            }
            else if (t.tur == ANAHTAR_KELIME && t.deger == "degilse") {
                ilerle(); bekleDeger(NOKTALAMA, "{", "'{' bekleniyor!");
                if (sonEgerDurumu) blokAtla();
                continue;
            }
            else if (t.tur == ANAHTAR_KELIME && t.deger == "dongu") {
                int donguBaslangic = pozisyon;
                ilerle(); bekleDeger(NOKTALAMA, "(", "'(' bekleniyor!");
                bool sart = mantiksalHesapla();
                bekleDeger(NOKTALAMA, ")", "')' bekleniyor!"); bekleDeger(NOKTALAMA, "{", "'{' bekleniyor!");
                if (sart) donguYigini.push_back(donguBaslangic);
                else blokAtla();
                continue;
            }
            // تعريف المتغيرات (محدث ليقبل متغيرات بدون قيمة ابتدائية)
            else if (t.tur == ANAHTAR_KELIME && (t.deger == "tam" || t.deger == "kesir" || t.deger == "metin")) {
                ilerle();
                Token isim = bekle(KIMLIK, "Degisken ismi bekleniyor!");

                // نفحص: هل يوجد يساوي أم فاصلة منقوطة فوراً؟
                Token sonraki = suankiToken();
                if (sonraki.tur == ISARET && sonraki.deger == "=") {
                    ilerle(); // نتخطى اليساوي
                    string sonuc = ifadeHesapla();
                    bekle(NOKTALAMA, "Satir sonunda ';' eksik!");
                    hafiza[isim.deger] = sonuc;
                }
                else if (sonraki.tur == NOKTALAMA && sonraki.deger == ";") {
                    ilerle(); // نتخطى الفاصلة المنقوطة
                    hafiza[isim.deger] = "0"; // نعطيه قيمة افتراضية صفر بالخفاء
                }
                else {
                    hataVer("Degisken tanimlarken '=' veya ';' bekleniyor!");
                }
            }

            // --- أمر الإدخال التفاعلي (oku) الجديد كلياً ---
            else if (t.tur == ANAHTAR_KELIME && t.deger == "oku") {
                ilerle();
                Token isim = bekle(KIMLIK, "Deger atanacak degisken ismi bekleniyor!");
                bekle(NOKTALAMA, "Satir sonunda ';' eksik!");

                // 1. نطبع رسالة تفاعلية ونجبر النظام على إرسالها فوراً للواجهة
                cout << "\n[SISTEM] Lutfen '" << isim.deger << "' icin bir deger girin:\n";
                cout.flush(); // سحري ومهم جداً لربطه بالواجهة

                // 2. ننتظر الإدخال الحقيقي من المستخدم
                string okunanDeger;
                if (cin >> okunanDeger) {
                    hafiza[isim.deger] = okunanDeger;
                }
                else {
                    hataVer("Girdi alinamadi!");
                }
            }

            else if (t.tur == ANAHTAR_KELIME && t.deger == "yazdir") {
                ilerle();
                Token isim = bekle(KIMLIK, "Yazdirilacak degisken ismi bekleniyor!");
                bekle(NOKTALAMA, "Satir sonunda ';' eksik!");
                if (hafiza.find(isim.deger) != hafiza.end()) cout << "Cikti >> " << hafiza[isim.deger] << endl;
                else hataVer("Tanimlanmayan degisken: " + isim.deger);
            }
            else if (t.tur == KIMLIK) {
                Token isim = t;
                ilerle();
                if (hafiza.find(isim.deger) == hafiza.end()) hataVer("Tanimlanmayan degisken: " + isim.deger);
                bekle(ISARET, "'=' isareti bekleniyor!");
                string sonuc = ifadeHesapla();
                bekle(NOKTALAMA, "Satir sonunda ';' eksik!");
                hafiza[isim.deger] = sonuc;
            }
            else {
                hataVer("Gecersiz komut: " + t.deger);
            }
        }
    }
};

// ==========================================
// 4. نقطة الانطلاق
// ==========================================
int main(int argc, char* argv[]) {
    string dosyaYolu = "program.trk";
    if (argc > 1) dosyaYolu = argv[1];

    ifstream dosya(dosyaYolu);
    if (!dosya.is_open()) {
        cout << "[!] HATA: '" << dosyaYolu << "' adli dosya bulunamadi!" << endl;
        return 1;
    }

    stringstream buffer;
    buffer << dosya.rdbuf();
    string kod = buffer.str();
    dosya.close();

    cout << "--- CayScript Programlama Dili Calisiyor --- [Build Final]\n";
    cout << "Okunan Dosya: " << dosyaYolu << "\n";
    cout << "-----------------------------------------\n";

    Lexer lexer(kod);
    vector<Token> tokenler = lexer.tokenleriAyir();

    Yorumlayici yorumlayici(tokenler);
    yorumlayici.calistir();

    cout << "-----------------------------------------\n";
    return 0;
}