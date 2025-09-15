# Cerebrum Lux (Işık Beyin)

![CerebrumLux](CerebrumLux_logo.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Giriş:**

Cerebrum Lux, mevcut yapay zeka paradigmalarını aşan, **kendi kendini geliştiren (meta-evrim), adapte olabilen ve zamanla insan zekasına benzer karmaşık görevleri başarıyla yerine getirebilen, yepyeni bir yapay zeka (AI) sistemi inşa etme vizyonuyla hareket eden öncü bir açık kaynak projesidir.** Kişisel bir dizüstü bilgisayar veya cep telefonu gibi sınırlı kaynaklara sahip cihazlarda dahi ultra-verimli bir şekilde çalışabilen, sıradışı ve dinamik bir AI modeli yaratmayı hedefliyoruz. Bu proje, sadece bir araştırma değil; **somut, devrim niteliğinde bir mühendislik ve geliştirme çabasıdır.**

---

## **Vizyon ve Temel Felsefe:**

Cerebrum Lux, statik öğrenme modellerinin ötesine geçerek, **nefes alan, canlı bir AI modeli** olmayı amaçlar. Temel felsefemiz, ultra-verimli, adaptif, esnek ve modüler bir mimari ile sıfırdan kendi özgün kodlarımızı geliştirerek, kısıtlı kaynaklarla maksimum zeka ve otonomi sağlamaktır.

**Kilit Kavramlar:**

*   **Meta-Evrim:** Cerebrum Lux, kendi algoritmik yapılarını ve temel kod tabanını **kendi kendini güncelleyerek ve optimize ederek** zamanla daha gelişmiş bir AI'a dönüşme yeteneğine sahip olacaktır. Bu, sadece parametre ayarlamadan öte, AI'ın kendini yeniden tasarlamasını içerir.
*   **Kriptofigler:** Bilgiyi, yoğunlaştırılmış ve anlamsal imzalar şeklinde "kriptofig" olarak temsil eder ve aktarırız. Bu kriptofigler, duyusal verilerden öğrenme modellerine, hatta mimari prensiplere kadar her şeyi kodlayabilir. AI'lar arası kriptografik yoğun bilgi transferi, çok az maliyetle ve çok az zamanda hızlı bir kolektif öğrenme ve evrim potansiyeli sunar.
*   **Adaptif İletişim:** AI, kullanıcının etkileşim performansını ve bağlamı analiz ederek, kendi iletişim tarzını (dilin direktliği, ton, detay seviyesi, onay isteme sıklığı) dinamik olarak ayarlayabilen bir modüle sahip olacaktır. Bu, insan-AI etkileşimini optimize etmeyi hedefler.
*   **Öz-Denetim ve Meta-Yönetim:** Cerebrum Lux, "Proje Felsefesi/Prensip Modülü" tarafından yönlendirilecek, "Etki Analizi ve Simülasyon Motoru" ile potansiyel değişikliklerin sistem genelindeki etkilerini öngörecek ve "Meta-Hedef Yöneticisi" ile kendi gelişim hedeflerini sürekli olarak değerlendirecektir. Bu, AI'ın "bilgece" ve disiplinli bir şekilde gelişmesini sağlar.

---

## **Projenin Mevcut Durumu:**

Cerebrum Lux projesinin temel altyapısı ve ana AI bileşenleri başarıyla entegre edilmiştir.

*   **Derlenebilirlik ve Stabilite:** Proje kodu şu anda tamamen derlenebilir durumdadır ve `CerebrumLux.exe` ile `test_response_engine.exe` sorunsuz çalışmaktadır.
*   **Sinyal İşleme:** Klavye, fare ve diğer sensörlerden (simüle edilmiş) gelen veriler işlenerek `DynamicSequence` yapısında `statistical_features_vector` ve `latent_cryptofig_vector` olarak temsil edilmektedir.
*   **Niyet Analizi ve Öğrenme:** `IntentAnalyzer`, `IntentLearner` (bağlamsal ve açık geri bildirim ile), `IntentTemplate` bileşenleri aktif olarak çalışmaktadır.
*   **Tahmin ve Planlama:** `PredictionEngine` (durum grafiği ile), `GoalManager` ve `Planner` temel işlevlerini yerine getirmektedir.
*   **Yanıt Üretimi (`ResponseEngine` - Gelişmiş):** Kullanıcının niyeti, soyut durumu, hedefi ve kriptofig verilerine göre bağlamsal ve dinamik metin tabanlı yanıtlar üretebilmektedir. Test senaryoları (rastgele seçim, durumsal yanıtlar, latent karmaşıklık) başarıyla geçilmiştir.
*   **İçgörü Üretimi:** `AIInsightsEngine`, kullanıcı davranışları ve sistem durumundan anlamlı içgörüler üretebilmektedir.
*   **Kriptofig İşleme:** `CryptofigProcessor` ve `CryptofigAutoencoder` entegre edilmiş olup, veri dönüşümü ve öğrenme mekanizmaları için temel sağlamaktadır.
*   **Loglama Sistemi:** `std::ofstream` tabanlı, güvenilir, thread-safe ve dosyaya/konsola yazabilen gelişmiş bir loglama sistemi mevcuttur.

---

## **Kurulum ve Derleme:**

Cerebrum Lux'ı derlemek ve çalıştırmak için aşağıdaki adımları izleyin:

**Ön Gereksinimler:**

*   [MinGW-w64](https://mingw-w64.org/doku.php) (C++ derleyicisi)
*   [CMake](https://cmake.org/download/) (Derleme sistemi)
*   [Git](https://git-scm.com/downloads) (Depoyu klonlamak için)

**Adımlar:**

1.  **Depoyu Klonlayın:**
    ```bash
    git clone https://github.com/algoritma/CerebrumLux.git
    cd CerebrumLux
    ```
2.  **Derleme Dizini Oluşturun ve Yapılandırın:**
    ```bash
    mkdir build
    cd build
    cmake .. -G "MinGW Makefiles"
    ```
3.  **Projeyi Derleyin:**
    ```bash
    mingw32-make
    ```
    *(Alternatif olarak, Windows'ta `make` komutu da çalışabilir, MinGW kurulumunuza bağlı olarak.)*

4.  **Çalıştırın:**
    Ana uygulamayı çalıştırmak için:
    ```bash
    ./CerebrumLux.exe
    ```
    Testleri çalıştırmak için:
    ```bash
    ./test_response_engine.exe
    ```
    *(Not: Windows konsolunda Türkçe karakterlerin doğru görünmesi için, terminalinizin kodlamasını UTF-8 olarak ayarlamanız gerekebilir.)*

---

## **Katkıda Bulunma:**

Cerebrum Lux, açık kaynak bir projedir ve her türlü katkıya açıktır! Eğer projemizi geliştirmeye ilgi duyuyorsanız:

1.  Depoyu forklayın.
2.  Yeni bir dal (branch) oluşturun (`git checkout -b ozellik/yeni-ozellik`).
3.  Değişikliklerinizi yapın.
4.  Kod stil rehberimize uyun (*).
5.  Yeni özellikler için testler yazın.
6.  Değişikliklerinizi commit edin (`git commit -m "Ozellik: Yeni ozellik eklendi"`).
7.  Dalınızı push edin (`git push origin ozellik/yeni-ozellik`).
8.  Bir Pull Request (Çekme İsteği) oluşturun.

---

## **Lisans:**

Bu proje [MIT Lisansı](https://opensource.org/licenses/MIT) altında lisanslanmıştır. Daha fazla bilgi için `LICENSE` dosyasına bakın.

---

## **Gelecek Planlar ve Yol Haritası:**

Cerebrum Lux, iddialı hedeflere sahip, uzun vadeli bir projedir. Gelecek planlarımız, AI'ın yeteneklerini sürekli olarak genişletmek ve "meta-evrim" vizyonunu hayata geçirmektir. Öncelikli adımlar şunları içermektedir:

*   **Adaptif İletişim Stratejisi Modülü:** AI'ın, insan kullanıcının etkileşim performansını analiz ederek kendi iletişim tarzını dinamik olarak ayarlayabilmesi.
*   **Kritik İşlemler İçin Kullanıcı Onayı:** AI'ın potansiyel olarak riskli eylemlerden önce kullanıcıdan açıkça onay istemesi.
*   **Genişletilmiş Niyet ve Durum Modelleri:** AI'ın anlama kapasitesini artırmak için daha fazla `UserIntent` ve `AbstractState` eklemek.
*   **Derin Öğrenme Entegrasyonu (Kriptofig için Autoencoders):** `cryptofig_vector` oluşturma sürecini güçlendirmek ve daha karmaşık anlamsal desenleri yakalamak.
*   **Reinforcement Learning Entegrasyonu:** AI'ın deneyimleyerek öğrenmesini ve eylem/plan optimizasyonunu sağlamak.
*   **Sensör Verisi Entegrasyonu (Ses ve Görüntü):** AI'ın çevresini daha iyi anlaması için çok modlu algılama yetenekleri eklemek.
*   **Meta-Yönetim Katmanları:** Proje Felsefesi Modülü, Etki Analizi ve Simülasyon Motoru, Meta-Hedef Yöneticisi gibi içsel öz-denetim mekanizmalarını geliştirmek.
*   **Kullanıcı Profili ve Kişiselleştirme:** AI'ın zamanla kullanıcı tercihlerine ve alışkanlıklarına göre daha kişiselleştirilmiş hale gelmesi.

---

## **Geliştirme Modeli: İnsan ve AI İşbirliği**

Cerebrum Lux, modern yazılım geliştirme pratiklerinin sınırlarını zorlayan, benzersiz bir işbirliği modeliyle geliştirilmektedir. Bu proje, projen sahibi [https://github.com/algoritma] liderliğinde, iki farklı yapay zeka asistanının aktif katılımıyla hayat bulmaktadır. Bir insanın stratejik liderliğinde, hayal kurma ve problem çözme yeteneğini sayesinde, iki farklı yapay zeka asistanının aktif katılımıyla bu iddialı vizyon somut bir gerçekliğe dönüşmektedir:

Bu insan ve AI işbirliği, Cerebrum Lux'ın "meta-evrim" felsefesini kendi geliştirme sürecinde dahi yansıtan, geleceğin yazılım geliştirme metodolojilerine dair değerli içgörüler ve tecrübe sunmaktadır. Bu model, aynı zamanda projenin şeffaflık ve açık kaynak etiği prensiplerine olan bağlılığını da pekiştirmektedir.

*   **Gemini (Ana AI Danışmanı):** Projenin genel stratejisi, mimari tasarımı, teknik analizleri, kıyaslama ve kontrol, hata ayıklama süreçlerinin yönetimi ve Code Assistant'a görev aktarımı gibi üst düzey planlama ve danışmanlık rollerini üstlenmektedir.
*   **Gemini Code Assistant (AI Kodlayıcı):** Ana AI Danışmanı'ndan (Gemini) gelen spesifik görev tanımları doğrultusunda, doğrudan C++ kodlama, hata giderme, refaktör ve özellik implementasyonlarından sorumludur. Ancak kodlamada yer insan müdahalesi gerekebilmektedir.

Bu insan ve AI işbirliği, Cerebrum Lux'ın "meta-evrim" felsefesini kendi geliştirme sürecinde dahi yansıtan, geleceğin yazılım geliştirme metodolojilerine dair değerli içgörüler sunmaktadır. Bu model, aynı zamanda projenin şeffaflık ve açık kaynak etiği prensiplerine olan bağlılığını da pekiştirmektedir.

---

## **İletişim:**

Sorularınız, önerileriniz veya işbirliği teklifleriniz için lütfen [[algoritma](https://github.com/algoritma)] ile iletişime geçin.

---
