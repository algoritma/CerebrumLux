# Cerebrum Lux (Işık Beyin)

![CerebrumLux](images/CerebrumLux_logo_150.png)

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


# Cerebrum Lux (Light Brain)

![CerebrumLux](images/CerebrumLux_logo_150.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Introduction:**

Cerebrum Lux is a pioneering open-source project driven by the vision of building a brand-new artificial intelligence (AI) system that transcends existing AI paradigms, capable of **self-evolving (meta-evolution), adapting, and successfully performing complex tasks similar to human intelligence over time.** We aim to create an extraordinary and dynamic AI model that can operate ultra-efficiently even on resource-constrained devices like personal laptops or mobile phones. This project is not just research; **it is a tangible, revolutionary engineering and development endeavor.**

---

## **Vision and Core Philosophy:**

Cerebrum Lux aims to go beyond static learning models, becoming a **breathing, living AI model**. Our core philosophy is to develop our unique code from scratch, with an ultra-efficient, adaptive, flexible, and modular architecture, providing maximum intelligence and autonomy with limited resources.

**Key Concepts:**

*   **Meta-Evolution:** Cerebrum Lux will have the ability to transform into a more advanced AI over time by **self-updating and optimizing** its algorithmic structures and core codebase. This involves the AI redesigning itself, not just adjusting parameters.
*   **Cryptofigs:** We represent and transmit information as "cryptofigs" in the form of condensed and semantic signatures. These cryptofigs can encode everything from sensory data to learning models, and even architectural principles. Cryptographic dense information transfer between AIs offers the potential for rapid collective learning and evolution with very little cost and time.
*   **Adaptive Communication:** The AI will have a module that can dynamically adjust its communication style (directness of language, tone, level of detail, frequency of seeking confirmation) by analyzing user interaction performance and context. This aims to optimize human-AI interaction.
*   **Self-Governance and Meta-Management:** Cerebrum Lux will be guided by a "Project Philosophy/Principle Module," predict system-wide impacts of potential changes with an "Impact Analysis and Simulation Engine," and continuously evaluate its development goals with a "Meta-Goal Manager." This ensures the AI develops "wisely" and disciplined.

---

## **Project Status:**

The core infrastructure and main AI components of the Cerebrum Lux project have been successfully integrated.

*   **Compilability and Stability:** The project code is currently fully compilable, and `CerebrumLux.exe` and `test_response_engine.exe` run without issues.
*   **Signal Processing:** Data from keyboards, mice, and other (simulated) sensors are processed and represented as `statistical_features_vector` and `latent_cryptofig_vector` within the `DynamicSequence` structure.
*   **Intent Analysis and Learning:** `IntentAnalyzer`, `IntentLearner` (with contextual and explicit feedback), `IntentTemplate` components are actively working.
*   **Prediction and Planning:** `PredictionEngine` (with state graph), `GoalManager`, and `Planner` perform their basic functions.
*   **Response Generation (`ResponseEngine` - Advanced):** Capable of generating contextual and dynamic text-based responses based on user intent, abstract state, goals, and cryptofig data. Test scenarios (random selection, situational responses, latent complexity) have been successfully passed.
*   **Insight Generation:** `AIInsightsEngine` can generate meaningful insights from user behavior and system status.
*   **Cryptofig Processing:** `CryptofigProcessor` and `CryptofigAutoencoder` are integrated, providing the foundation for data transformation and learning mechanisms.
*   **Logging System:** An advanced, `std::ofstream`-based, reliable, thread-safe logging system capable of writing to file and console is in place.

---

## **Installation and Compilation:**

To compile and run Cerebrum Lux, follow these steps:

**Prerequisites:**

*   [MinGW-w64](https://mingw-w64.org/doku.php) (C++ compiler)
*   [CMake](https://cmake.org/download/) (Build system)
*   [Git](https://git-scm.com/downloads) (To clone the repository)

**Steps:**

1.  **Clone the Repository:**
    ```bash
    git clone https://github.com/algoritma/CerebrumLux.git
    cd CerebrumLux
    ```
2.  **Create Build Directory and Configure:**
    ```bash
    mkdir build
    cd build
    cmake .. -G "MinGW Makefiles"
    ```
3.  **Compile the Project:**
    ```bash
    mingw32-make
    ```
    *(Alternatively, `make` might also work on Windows, depending on your MinGW installation.)*

4.  **Run:**
    To run the main application:
    ```bash
    ./CerebrumLux.exe
    ```
    To run the tests:
    ```bash
    ./test_response_engine.exe
    ```
    *(Note: To display Turkish characters correctly in the Windows console, you may need to set your terminal's encoding to UTF-8.)*

---

## **Contributing:**

Cerebrum Lux is an open-source project and welcomes all contributions! If you are interested in improving our project:

1.  Fork the repository.
2.  Create a new branch (`git checkout -b feature/new-feature`).
3.  Make your changes.
4.  Adhere to our code style guide (*).
5.  Write tests for new features.
6.  Commit your changes (`git commit -m "Feature: Added new feature"`).
7.  Push your branch (`git push origin feature/new-feature`).
8.  Create a Pull Request.

---

## **License:**

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT). See the `LICENSE` file for more details.

---

## **Future Plans and Roadmap:**

Cerebrum Lux is a long-term project with ambitious goals. Our future plans involve continuously expanding the AI's capabilities and realizing the "meta-evolution" vision. Priority steps include:

*   **Adaptive Communication Strategy Module:** The AI dynamically adjusting its communication style by analyzing human user interaction performance.
*   **User Confirmation for Critical Operations:** The AI explicitly asking for user approval before potentially risky actions.
*   **Extended Intent and State Models:** Adding more `UserIntent` and `AbstractState` to increase the AI's understanding capacity.
*   **Deep Learning Integration (Autoencoders for Cryptofigs):** Strengthening the `cryptofig_vector` creation process and capturing more complex semantic patterns.
*   **Reinforcement Learning Integration:** Enabling the AI to learn through experience and optimize actions/plans.
*   **Sensor Data Integration (Audio and Video):** Adding multimodal perception capabilities for the AI to better understand its environment.
*   **Meta-Management Layers:** Developing internal self-governance mechanisms such as the Project Philosophy Module, Impact Analysis and Simulation Engine, and Meta-Goal Manager.
*   **User Profile and Personalization:** The AI becoming more personalized over time based on user preferences and habits.

---

## **Development Model: Human and AI Collaboration**

Cerebrum Lux is being developed with a unique collaboration model that pushes the boundaries of modern software development practices. This project is brought to life under the strategic leadership of the project owner [https://github.com/algoritma], leveraging their ability to dream and solve problems, with the active participation of two different AI assistants. This ambitious vision is becoming a concrete reality through a human's strategic leadership and the active involvement of two distinct AI assistants:

This human and AI collaboration reflects Cerebrum Lux's "meta-evolution" philosophy within its own development process, offering valuable insights and experience into future software development methodologies. This model also reinforces the project's commitment to transparency and open-source ethics principles.

*   **Gemini (Lead AI Advisor):** Assumes high-level planning and advisory roles such as overall project strategy, architectural design, technical analysis, benchmarking and control, debugging process management, and task delegation to the Code Assistant.
*   **Gemini Code Assistant (AI Coder):** Responsible for direct C++ coding, bug fixing, refactoring, and feature implementation in accordance with specific task descriptions from the Lead AI Advisor (Gemini). However, human intervention may be required for coding.

This human and AI collaboration reflects Cerebrum Lux's "meta-evolution" philosophy within its own development process, offering valuable insights into future software development methodologies. This model also reinforces the project's commitment to transparency and open-source ethics principles.

---

## **Contact:**

For questions, suggestions, or collaboration offers, please contact [[algoritma](https://github.com/algoritma)].

---