#pragma once
#define NO_S3D_USING
//#define NOMINMAX
#include <Siv3D.hpp>  // OpenSiv3D v0.4.3
#include <LoadBalancing-cpp/inc/Client.h>

using s3d::int32;
using s3d::uint32;
using s3d::uint64;


namespace Utility {
    /// <summary>
    /// ExitGames::Common::JStringからs3d::Stringに変換する
    /// </summary>
    /// <param name="str">変換したい文字列</param>
    /// <returns>s3d::Stringに変換した文字列</returns>
    [[nodiscard]] inline s3d::String ConvertJStringToString(const ExitGames::Common::JString& str) {
        return s3d::Unicode::FromWString(std::wstring(str));
    }

    /// <summary>
    /// s3d::StringからExitGames::Common::JStringに変換する
    /// </summary>
    /// <param name="str">変換したい文字列</param>
    /// <returns>ExitGames::Common::JStringに変換した文字列</returns>
    [[nodiscard]] inline ExitGames::Common::JString ConvertStringToJString(const s3d::String& str) {
        return ExitGames::Common::JString(str.toWstr().c_str());
    }

    /// <summary>
    /// appIDを正常な文字列に直す
    /// </summary>
    /// <param name=str>修正前のappID</param>
    /// <returns>正常なappID</returns>
    /// <remarks>
    /// ここには実装部分は書かないし、gitにも履歴は残さない
    /// </remarks>
    [[nodiscard]] ExitGames::Common::JString ChangeAppIDString(s3d::String str);

    /// <summary>
    /// シーン管理
    /// </summary>
    /// <remarks>
    /// State にはシーンを区別するキーの型、Data にはシーン間で共有するデータの型を指定します。
    /// </remarks>
    template<class State, class Data>
    class SceneMaster;

    namespace detail {
        struct EmptyData {};
    }  // namespace detail

    /// <summary>
    /// シーン・インタフェース
    /// </summary>
    template<class State, class Data>
    class IScene : s3d::Uncopyable {
    public:
        using State_t = State;

        using Data_t = Data;

        struct InitData {
            State_t state;

            std::shared_ptr<Data_t> _s;

            SceneMaster<State_t, Data_t>* _m;

            InitData() = default;

            InitData(const State_t& _state, const std::shared_ptr<Data_t>& data, SceneMaster<State_t, Data_t>* manager) : state(_state), _s(data), _m(manager) {}
        };

    private:
        State_t m_state;

        std::shared_ptr<Data_t> m_data;

        SceneMaster<State_t, Data_t>* m_manager;

    public:
        virtual void DebugReturn(int /*debugLevel*/, const ExitGames::Common::JString& /*string*/) {}

        virtual void ConnectionErrorReturn(int /*errorCode*/) {}

        virtual void ClientErrorReturn(int /*errorCode*/) {}

        virtual void WarningReturn(int /*warningCode*/) {}

        virtual void ServerErrorReturn(int /*errorCode*/) {}

        virtual void JoinRoomEventAction(int /*playerNr*/, const ExitGames::Common::JVector<int>& /*playernrs*/, const ExitGames::LoadBalancing::Player& /*player*/) {}

        virtual void LeaveRoomEventAction(int /*playerNr*/, bool /*isInactive*/) {}

        virtual void CustomEventAction(int /*playerNr*/, nByte /*eventCode*/, const ExitGames::Common::Object& /*eventContent*/) {}

        virtual void ConnectReturn(int /*errorCode*/,
                                   const ExitGames::Common::JString& /*errorString*/,
                                   const ExitGames::Common::JString& /*region*/,
                                   const ExitGames::Common::JString& /*cluster*/) {}

        virtual void DisconnectReturn() {}

        virtual void LeaveRoomReturn(int /*errorCode*/, const ExitGames::Common::JString& /*errorString*/) {}

        virtual void CreateRoomReturn(int /*localPlayerNr*/,
                                      const ExitGames::Common::Hashtable& /*roomProperties*/,
                                      const ExitGames::Common::Hashtable& /*playerProperties*/,
                                      int /*errorCode*/,
                                      const ExitGames::Common::JString& /*errorString*/) {}

        virtual void JoinRandomRoomReturn(int /*localPlayerNr*/,
                                          const ExitGames::Common::Hashtable& /*roomProperties*/,
                                          const ExitGames::Common::Hashtable& /*playerProperties*/,
                                          int /*errorCode*/,
                                          const ExitGames::Common::JString& /*errorString*/) {}

    public:
        explicit IScene(const InitData& init) : m_state(init.state), m_data(init._s), m_manager(init._m) {}

        virtual void Connect() {
            m_manager->GetClient().setAutoJoinLobby(true);

            if (!m_manager->GetClient().connect(ExitGames::LoadBalancing::AuthenticationValues().setUserID(ExitGames::Common::JString() + GETTIMEMS()))) {
                return;
            }

            m_manager->UsePhoton(true);
        }

        virtual void Disconnect() {
            m_manager->GetClient().disconnect();
        }

        virtual ~IScene() {}

        virtual void UpdatePhoton() {}

        virtual void RunPhoton() {
            UpdatePhoton();
            m_manager->GetClient().service();
        }

        virtual void CreateRoom(const ExitGames::Common::JString& roomName_, const ExitGames::Common::Hashtable& properties_, const nByte maxPlayers_) {
            m_manager->GetClient().opCreateRoom(roomName_, ExitGames::LoadBalancing::RoomOptions().setMaxPlayers(maxPlayers_).setCustomRoomProperties(properties_));
        }

        ExitGames::LoadBalancing::Client& GetClient() {
            return m_manager->GetClient();
        }

        /// <summary>
        /// フェードイン時の更新
        /// </summary>
        /// <returns>
        /// なし
        /// </returns>
        virtual void updateFadeIn(double) {}

        /// <summary>
        /// 通常時の更新
        /// </summary>
        /// <returns>
        /// なし
        /// </returns>
        virtual void update() {}

        /// <summary>
        /// フェードアウト時の更新
        /// </summary>
        /// <returns>
        /// なし
        /// </returns>
        virtual void updateFadeOut(double) {}

        /// <summary>
        /// 通常時の描画
        /// </summary>
        /// <returns>
        /// なし
        /// </returns>
        virtual void draw() const {}

        /// <summary>
        /// フェードイン時の描画
        /// </summary>
        /// <param name="t">
        /// フェードインの経過 (0.0 → 1.0)
        /// </param>
        /// <returns>
        /// なし
        /// </returns>
        virtual void drawFadeIn(double t) const {
            draw();

            s3d::Transformer2D transform(s3d::Mat3x2::Identity(), s3d::Transformer2D::Target::SetLocal);

            s3d::Scene::Rect().draw(s3d::ColorF(m_manager->getFadeColor()).setA(1.0 - t));
        }

        /// <summary>
        /// フェードイアウト時の描画
        /// </summary>
        /// <param name="t">
        /// フェードアウトの経過 (0.0 → 1.0)
        /// </param>
        /// <returns>
        /// なし
        /// </returns>
        virtual void drawFadeOut(double t) const {
            draw();

            s3d::Transformer2D transform(s3d::Mat3x2::Identity(), s3d::Transformer2D::Target::SetLocal);

            s3d::Scene::Rect().draw(s3d::ColorF(m_manager->getFadeColor()).setA(t));
        }

    protected:
        [[nodiscard]] const State_t& getState() const {
            return m_state;
        }

        /// <summary>
        /// 共有データへの参照を取得します。
        /// </summary>
        /// <returns>
        /// 共有データへの参照
        /// </returns>
        [[nodiscard]] Data_t& getData() const {
            return *m_data;
        }

        /// <summary>
        /// シーンの変更を通知します。
        /// </summary>
        /// <param name="state">
        /// 次のシーンのキー
        /// </param>
        /// <param name="transitionTime">
        /// フェードイン・アウトの時間
        /// </param>
        /// <param name="crossFade">
        /// クロスフェードを有効にするか
        /// </param>
        /// <returns>
        /// シーンの変更が可能でフェードイン・アウトが開始される場合 true, それ以外の場合は false
        /// </returns>
        bool changeScene(const State_t& state, const s3d::Duration& transitionTime = s3d::MillisecondsF(1000), bool crossFade = false) {
            return changeScene(state, static_cast<s3d::int32>(transitionTime.count() * 1000), crossFade);
        }

        /// <summary>
        /// シーンの変更を通知します。
        /// </summary>
        /// <param name="state">
        /// 次のシーンのキー
        /// </param>
        /// <param name="transitionTimeMillisec">
        /// フェードイン・アウトの時間（ミリ秒）
        /// </param>
        /// <param name="crossFade">
        /// クロスフェードを有効にするか
        /// </param>
        /// <returns>
        /// シーンの変更が可能でフェードイン・アウトが開始される場合 true, それ以外の場合は false
        /// </returns>
        bool changeScene(const State_t& state, s3d::int32 transitionTimeMillisec, bool crossFade = false) {
            return m_manager->changeScene(state, transitionTimeMillisec, crossFade);
        }

        /// <summary>
        /// エラーの発生を通知します。
        /// </summary>
        /// <remarks>
        /// この関数を呼ぶと、以降の SceneMaster::update() / updateAndDraw() が false を返します。
        /// </remarks>
        /// <returns>
        /// なし
        /// </returns>
        void notifyError() {
            return m_manager->notifyError();
        }
    };

    /// <summary>
    /// シーン管理
    /// </summary>
    /// <remarks>
    /// State にはシーンを区別するキーの型、Data にはシーン間で共有するデータの型を指定します。
    /// </remarks>
    template<class State, class Data = detail::EmptyData>
    class SceneMaster : public ExitGames::LoadBalancing::Listener {
    private:
        using Scene_t = std::shared_ptr<IScene<State, Data>>;

        using FactoryFunction_t = std::function<Scene_t()>;

        s3d::HashTable<State, FactoryFunction_t> m_factories;

        std::shared_ptr<Data> m_data;

        Scene_t m_current;

        Scene_t m_next;

        Scene_t m_previous;

        State m_currentState;

        State m_nextState;

        s3d::Optional<State> m_first;

        enum class TransitionState {
            None,

            FadeIn,

            Active,

            FadeOut,

            FadeInOut,

        } m_transitionState
            = TransitionState::None;

        s3d::Stopwatch m_stopwatch;

        s3d::int32 m_transitionTimeMillisec = 1000;

        s3d::ColorF m_fadeColor = s3d::Palette::Black;

        bool m_crossFade = false;

        bool m_error = false;

        bool m_usePhoton;

        ExitGames::LoadBalancing::Client m_loadBalancingClient;

        bool updateSingle() {
            double elapsed = m_stopwatch.msF();

            if (m_transitionState == TransitionState::FadeOut && elapsed >= m_transitionTimeMillisec) {
                m_current = nullptr;

                m_current = m_factories[m_nextState]();

                if (hasError()) {
                    return false;
                }

                m_currentState = m_nextState;

                m_transitionState = TransitionState::FadeIn;

                m_stopwatch.restart();

                elapsed = 0.0;
            }

            if (m_transitionState == TransitionState::FadeIn && elapsed >= m_transitionTimeMillisec) {
                m_stopwatch.reset();

                m_transitionState = TransitionState::Active;
            }

            switch (m_transitionState) {
            case TransitionState::FadeIn:
                assert(m_transitionTimeMillisec);
                m_current->updateFadeIn(elapsed / m_transitionTimeMillisec);
                return !hasError();
            case TransitionState::Active:
                m_current->update();
                if (UsePhoton()) {
                    m_current->RunPhoton();
                }
                return !hasError();
            case TransitionState::FadeOut:
                assert(m_transitionTimeMillisec);
                m_current->updateFadeOut(elapsed / m_transitionTimeMillisec);
                return !hasError();
            default:
                return false;
            }
        }

        bool updateCross() {
            const double elapsed = m_stopwatch.msF();

            if (m_transitionState == TransitionState::FadeInOut) {
                if (elapsed >= m_transitionTimeMillisec) {
                    m_current = m_next;

                    m_next = nullptr;

                    m_stopwatch.reset();

                    m_transitionState = TransitionState::Active;
                }
            }

            if (m_transitionState == TransitionState::Active) {
                m_current->update();

                return !hasError();
            }
            else {
                assert(m_transitionTimeMillisec);

                const double t = elapsed / m_transitionTimeMillisec;

                m_current->updateFadeOut(t);

                if (hasError()) {
                    return false;
                }

                m_next->updateFadeIn(t);

                return !hasError();
            }
        }

        [[nodiscard]] bool hasError() const noexcept {
            return m_error;
        }

    public:
        /// <summary>
        /// シーンのインタフェース
        /// </summary>
        using Scene = IScene<State, Data>;

        /// <summary>
        /// シーン管理を初期化します。
        /// </summary>
        /// <param name="option">
        /// シーン管理のオプション
        /// </param>
        SceneMaster(const ExitGames::Common::JString& appID_, const ExitGames::Common::JString& appVersion_)
        : m_data(s3d::MakeShared<Data>()), m_loadBalancingClient(*this, appID_, appVersion_), m_usePhoton(false) {}

        /// <summary>
        /// シーン管理を初期化します。
        /// </summary>
        /// <param name="data">
        /// 共有データ
        /// </param>
        /// <param name="option">
        /// シーン管理のオプション
        /// </param>
        explicit SceneMaster(const std::shared_ptr<Data>& data, const ExitGames::Common::JString& appID_, const ExitGames::Common::JString& appVersion_)
        : m_data(data), m_loadBalancingClient(*this, appID_, appVersion_), m_usePhoton(false) {}

        ~SceneMaster() {
            if (UsePhoton()) {
                m_loadBalancingClient.disconnect();
            }
        }

        bool UsePhoton() {
            return m_usePhoton;
        }
        void UsePhoton(const bool use_) {
            m_usePhoton = use_;
        }

        /// <summary>
        /// シーンを追加します。
        /// </summary>
        /// <param name="state">
        /// シーンのキー
        /// </param>
        /// <returns>
        /// 追加に成功した場合 true, それ以外の場合は false
        /// </returns>
        template<class Scene>
        SceneMaster& add(const State& state) {
            typename Scene::InitData initData(state, m_data, this);

            auto factory = [=]() { return std::make_shared<Scene>(initData); };

            auto it = m_factories.find(state);

            if (it != m_factories.end()) {
                it.value() = factory;
            }
            else {
                m_factories.emplace(state, factory);

                if (!m_first) {
                    m_first = state;
                }
            }

            return *this;
        }

        /// <summary>
        /// 最初のシーンを初期化します。
        /// </summary>
        /// <param name="state">
        /// 最初のシーン
        /// </param>
        /// <returns>
        /// 初期化に成功した場合 true, それ以外の場合は false
        /// </returns>
        bool init(const State& state) {
            if (m_current) {
                return false;
            }

            auto it = m_factories.find(state);

            if (it == m_factories.end()) {
                return false;
            }

            m_currentState = state;

            m_current = it->second();

            if (hasError()) {
                return false;
            }

            m_transitionState = TransitionState::FadeIn;

            m_stopwatch.restart();

            return true;
        }

        /// <summary>
        /// シーンを更新します。
        /// </summary>
        /// <returns>
        /// シーンの更新に成功した場合 true, それ以外の場合は false
        /// </returns>
        bool updateScene() {
            if (hasError()) {
                return false;
            }

            if (!m_current) {
                if (!m_first) {
                    return true;
                }
                else if (!init(m_first.value())) {
                    return false;
                }
            }

            if (m_crossFade) {
                return updateCross();
            }
            else {
                return updateSingle();
            }
        }

        /// <summary>
        /// シーンを描画します。
        /// </summary>
        /// <returns>
        /// なし
        /// </returns>
        void drawScene() const {
            if (!m_current) {
                return;
            }

            if (m_transitionState == TransitionState::Active || !m_transitionTimeMillisec) {
                m_current->draw();
            }

            const double elapsed = m_stopwatch.msF();

            if (m_transitionState == TransitionState::FadeIn) {
                m_current->drawFadeIn(elapsed / m_transitionTimeMillisec);
            }
            else if (m_transitionState == TransitionState::FadeOut) {
                m_current->drawFadeOut(elapsed / m_transitionTimeMillisec);
            }
            else if (m_transitionState == TransitionState::FadeInOut) {
                m_current->drawFadeOut(elapsed / m_transitionTimeMillisec);

                if (m_next) {
                    m_next->drawFadeIn(elapsed / m_transitionTimeMillisec);
                }
            }
        }

        /// <summary>
        /// シーンの更新と描画を行います。
        /// </summary>
        /// <returns>
        /// シーンの更新に成功した場合 true, それ以外の場合は false
        /// </returns>
        bool update() {
            if (!updateScene()) {
                return false;
            }

            drawScene();

            return true;
        }

        /// <summary>
        /// 共有データを取得します。
        /// </summary>
        /// <returns>
        /// 共有データへのポインタ
        /// </returns>
        [[nodiscard]] std::shared_ptr<Data> get() {
            return m_data;
        }

        /// <summary>
        /// 共有データを取得します。
        /// </summary>
        /// <returns>
        /// 共有データへのポインタ
        /// </returns>
        [[nodiscard]] const std::shared_ptr<const Data> get() const {
            return m_data;
        }

        /// <summary>
        /// シーンを変更します。
        /// </summary>
        /// <param name="state">
        /// 次のシーンのキー
        /// </param>
        /// <param name="transitionTimeMillisec">
        /// フェードイン・アウトの時間（ミリ秒）
        /// </param>
        /// <param name="crossFade">
        /// クロスフェードを有効にするか
        /// </param>
        /// <returns>
        /// シーンの変更が可能でフェードイン・アウトが開始される場合 true, それ以外の場合は false
        /// </returns>
        bool changeScene(const State& state, s3d::int32 transitionTimeMillisec, bool crossFade) {
            if (state == m_currentState) {
                crossFade = false;
            }

            if (m_factories.find(state) == m_factories.end()) {
                return false;
            }

            m_nextState = state;

            m_crossFade = crossFade;

            if (crossFade) {
                m_transitionTimeMillisec = transitionTimeMillisec;

                m_transitionState = TransitionState::FadeInOut;

                m_next = m_factories[m_nextState]();

                if (hasError()) {
                    return false;
                }

                m_currentState = m_nextState;

                m_stopwatch.restart();
            }
            else {
                m_transitionTimeMillisec = (transitionTimeMillisec / 2);

                m_transitionState = TransitionState::FadeOut;

                m_stopwatch.restart();
            }

            return true;
        }

        /// <summary>
        /// フェードイン・アウトのデフォルトの色を設定します。
        /// </summary>
        /// <param name="color">
        /// フェードイン・アウトのデフォルトの色
        /// </param>
        /// <returns>
        /// なし
        /// </returns>
        SceneMaster& setFadeColor(const s3d::ColorF& color) {
            m_fadeColor = color;
            return *this;
        }

        /// <summary>
        /// フェードイン・アウトのデフォルトの色を取得します。
        /// </summary>
        /// <returns>
        /// フェードイン・アウトのデフォルトの色
        /// </returns>
        const s3d::ColorF& getFadeColor() const {
            return m_fadeColor;
        }

        ExitGames::LoadBalancing::Client& GetClient() {
            return m_loadBalancingClient;
        }

        /// <summary>
        /// エラーの発生を通知します。
        /// </summary>
        /// <remarks>
        /// この関数を呼ぶと、以降の SceneMaster::update() / updateAndDraw() が false を返します。
        /// </remarks>
        /// <returns>
        /// なし
        /// </returns>
        void notifyError() {
            m_error = true;
        }

    private:
        virtual void debugReturn(int debugLevel, const ExitGames::Common::JString& string) override {
            if (m_current) {
                m_current->DebugReturn(debugLevel, string);
            }
        }

        virtual void connectionErrorReturn(int errorCode) override {
            m_current->ConnectionErrorReturn(errorCode);
        }

        virtual void clientErrorReturn(int errorCode) override {
            if (m_current) {
                m_current->ClientErrorReturn(errorCode);
            }
        }

        virtual void warningReturn(int warningCode) override {
            m_current->WarningReturn(warningCode);
        }

        virtual void serverErrorReturn(int errorCode) override {
            m_current->ServerErrorReturn(errorCode);
        }

        virtual void joinRoomEventAction(int playerNr, const ExitGames::Common::JVector<int>& playernrs, const ExitGames::LoadBalancing::Player& player) override {
            m_current->JoinRoomEventAction(playerNr, playernrs, player);
        }

        virtual void leaveRoomEventAction(int playerNr, bool isInactive) override {
            m_current->LeaveRoomEventAction(playerNr, isInactive);
        }

        virtual void customEventAction(int playerNr, nByte eventCode, const ExitGames::Common::Object& eventContent) override {
            m_current->CustomEventAction(playerNr, eventCode, eventContent);
        }

        virtual void connectReturn(int errorCode,
                                   const ExitGames::Common::JString& errorString,
                                   const ExitGames::Common::JString& region,
                                   const ExitGames::Common::JString& cluster) override {
            m_current->ConnectReturn(errorCode, errorString, region, cluster);
        }

        virtual void disconnectReturn() override {
            m_current->DisconnectReturn();
            m_usePhoton = false;
        }

        virtual void leaveRoomReturn(int errorCode, const ExitGames::Common::JString& errorString) override {
            m_current->LeaveRoomReturn(errorCode, errorString);
        }

        virtual void createRoomReturn(int localPlayerNr,
                                      const ExitGames::Common::Hashtable& roomProperties,
                                      const ExitGames::Common::Hashtable& playerProperties,
                                      int errorCode,
                                      const ExitGames::Common::JString& errorString) override {
            m_current->CreateRoomReturn(localPlayerNr, roomProperties, playerProperties, errorCode, errorString);
        }

        virtual void joinRandomRoomReturn(int localPlayerNr,
                                          const ExitGames::Common::Hashtable& roomProperties,
                                          const ExitGames::Common::Hashtable& playerProperties,
                                          int errorCode,
                                          const ExitGames::Common::JString& errorString) override {
            m_current->JoinRandomRoomReturn(localPlayerNr, roomProperties, playerProperties, errorCode, errorString);
        }
    };
}  // namespace shogi
