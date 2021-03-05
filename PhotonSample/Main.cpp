#include "SceneMaster.hpp"

/// <summary>
/// 共通データ
/// シーン間で引き継ぐ用
/// </summary>
namespace Common {
    enum class Scene {
        Title,  // タイトルシーン
        Match   // マッチングシーン
    };

    /// <summary>
    /// ゲームデータ
    /// </summary>
    class GameData {
    private:
        // ランダムルームのフィルター用
        ExitGames::Common::Hashtable m_hashTable;

    public:
        GameData() {
            m_hashTable.put(L"gameType", L"photonSample");
        }

        ExitGames::Common::Hashtable& GetCustomProperties() {
            return m_hashTable;
        }
    };
}

using MyScene = Utility::SceneMaster<Common::Scene, Common::GameData>;

namespace Sample {
    class Title : public MyScene::Scene {
    private:
        s3d::RectF m_startButton;
        s3d::RectF m_exitButton;

        s3d::Transition m_startButtonTransition;
        s3d::Transition m_exitButtonTransition;

    public:
        Title(const InitData& init_)
            : IScene(init_)
            , m_startButton(s3d::Arg::center(s3d::Scene::Center()), 300, 60)
            , m_exitButton(s3d::Arg::center(s3d::Scene::Center().movedBy(0, 100)), 300, 60)
            , m_startButtonTransition(s3d::SecondsF(0.4), s3d::SecondsF(0.2))
            , m_exitButtonTransition(s3d::SecondsF(0.4), s3d::SecondsF(0.2)) {}

        void update() override {
            m_startButtonTransition.update(m_startButton.mouseOver());
            m_exitButtonTransition.update(m_exitButton.mouseOver());

            if (m_startButton.mouseOver() || m_exitButton.mouseOver()) {
                s3d::Cursor::RequestStyle(s3d::CursorStyle::Hand);
            }

            if (m_startButton.leftClicked()) {
                changeScene(Common::Scene::Match);
                return;
            }

            if (m_exitButton.leftClicked()) {
                s3d::System::Exit();
                return;
            }
        }

        void draw() const override {
            const s3d::String titleText = U"Photonサンプル";
            const s3d::Vec2 center(s3d::Scene::Center().x, 120);
            s3d::FontAsset(U"Title")(titleText).drawAt(center.movedBy(4, 6), s3d::ColorF(0.0, 0.5));
            s3d::FontAsset(U"Title")(titleText).drawAt(center);

            m_startButton.draw(s3d::ColorF(1.0, m_startButtonTransition.value())).drawFrame(2);
            m_exitButton.draw(s3d::ColorF(1.0, m_exitButtonTransition.value())).drawFrame(2);

            s3d::FontAsset(U"Menu")(U"接続する").drawAt(m_startButton.center(), s3d::ColorF(0.25));
            s3d::FontAsset(U"Menu")(U"おわる").drawAt(m_exitButton.center(), s3d::ColorF(0.25));
        }
    };

    class Match : public MyScene::Scene {
    private:
        s3d::RectF m_exitButton;

        s3d::Transition m_exitButtonTransition;

        void ConnectReturn(int errorCode, const ExitGames::Common::JString& errorString, const ExitGames::Common::JString& region, const ExitGames::Common::JString& cluster) override {
            if (errorCode) {
                s3d::Print(U"接続出来ませんでした");
                changeScene(Common::Scene::Title);  // タイトルシーンに戻る
                return;
            }

            s3d::Print(U"接続しました");
            GetClient().opJoinRandomRoom(getData().GetCustomProperties(), 2);   // 第2引数でルームに参加できる人数を設定します。
        }

        void DisconnectReturn() override {
            s3d::Print(U"切断しました");
            changeScene(Common::Scene::Title);  // タイトルシーンに戻る
        }

        void CreateRoomReturn(int localPlayerNr,
            const ExitGames::Common::Hashtable& roomProperties,
            const ExitGames::Common::Hashtable& playerProperties,
            int errorCode,
            const ExitGames::Common::JString& errorString) override {
            if (errorCode) {
                s3d::Print(U"部屋を作成出来ませんでした");
                Disconnect();
                return;
            }

            s3d::Print(U"部屋を作成しました！");
        }

        void JoinRandomRoomReturn(int localPlayerNr,
            const ExitGames::Common::Hashtable& roomProperties,
            const ExitGames::Common::Hashtable& playerProperties,
            int errorCode,
            const ExitGames::Common::JString& errorString) override {
            if (errorCode) {
                s3d::Print(U"部屋が見つかりませんでした");

                CreateRoom(L"", getData().GetCustomProperties(), 2);

                s3d::Print(U"部屋を作成します...");
                return;
            }

            s3d::Print(U"部屋に接続しました!");
            // この下はゲームシーンに進んだり対戦相手が設定したりする処理を書きます。
            //changeScene(Common::Scene::Game);  // ゲームシーンに進む
        }

        void JoinRoomEventAction(int playerNr, const ExitGames::Common::JVector<int>& playernrs, const ExitGames::LoadBalancing::Player& player) override {
            // 部屋に入室したのが自分の場合、早期リターン
            if (GetClient().getLocalPlayer().getNumber() == player.getNumber()) {
                return;
            }

            s3d::Print(U"対戦相手が入室しました。");
            // この下はゲームシーンに進んだり設定したりする処理を書きます。
            //changeScene(Common::Scene::Game);  // ゲームシーンに進む
        }

        void LeaveRoomEventAction(int playerNr, bool isInactive) override {
            s3d::Print(U"対戦相手が退室しました。");
            changeScene(Common::Scene::Title);  // タイトルシーンに戻る
        }

        /// <summary>
        /// 今回はこの関数は必要ない(何も受信しない為)
        /// </summary>
        void CustomEventAction(int playerNr, nByte eventCode, const ExitGames::Common::Object& eventContent) override {
            s3d::Print(U"受信しました");
        }

    public:
        Match(const InitData& init_)
            : IScene(init_)
            , m_exitButton(s3d::Arg::center(s3d::Scene::Center().movedBy(300, 200)), 300, 60)
            , m_exitButtonTransition(s3d::SecondsF(0.4), s3d::SecondsF(0.2)) {
            Connect();  // この関数を呼び出せば接続できる。
            s3d::Print(U"接続中...");

            GetClient().fetchServerTimestamp();
        }

        void update() override {
            m_exitButtonTransition.update(m_exitButton.mouseOver());

            if (m_exitButton.mouseOver()) {
                s3d::Cursor::RequestStyle(s3d::CursorStyle::Hand);
            }

            if (m_exitButton.leftClicked()) {
                Disconnect();   // この関数を呼び出せば切断できる。
                return;
            }
        }

        void draw() const override {
            m_exitButton.draw(s3d::ColorF(1.0, m_exitButtonTransition.value())).drawFrame(2);

            s3d::FontAsset(U"Menu")(U"タイトルに戻る").drawAt(m_exitButton.center(), s3d::ColorF(0.25));
        }
    };
}

void Main() {
    // タイトルを設定
    s3d::Window::SetTitle(U"Photonサンプル");

    // ウィンドウの大きさを設定
    s3d::Window::Resize(1280, 720);

    // 背景色を設定
    s3d::Scene::SetBackground(s3d::ColorF(0.2, 0.8, 0.4));

    // 使用するフォントアセットを登録
    s3d::FontAsset::Register(U"Title", 120, s3d::Typeface::Heavy);
    s3d::FontAsset::Register(U"Menu", 30, s3d::Typeface::Regular);

    // シーンと遷移時の色を設定
    MyScene manager(L"/*ここにPhotonのappIDを入力してください。*/", L"1.0");

    manager.add<Sample::Title>(Common::Scene::Title)
        .add<Sample::Match>(Common::Scene::Match)
        .setFadeColor(s3d::ColorF(1.0));

    while (s3d::System::Update()) {
        if (!manager.update()) {
            break;
        }
    }
}
