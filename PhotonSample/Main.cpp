#include "SceneMaster.hpp"

/// <summary>
/// 共通データ
/// シーン間で引き継ぐ用
/// </summary>
namespace Common {
    enum class Scene {
        Title,  // タイトルシーン
        Match,  // マッチングシーン
        Game    // ゲームシーン
    };

    /// <summary>
    /// ゲームデータ
    /// </summary>
    class GameData {
    private:
        // ランダムルームのフィルター用
        ExitGames::Common::Hashtable m_hashTable;

        bool m_isMasterClient;

    public:
        GameData() : m_isMasterClient(false) {
            m_hashTable.put(L"gameType", L"photonSample");
        }

        ExitGames::Common::Hashtable& GetCustomProperties() {
            return m_hashTable;
        }

        [[nodiscard]] bool IsMasterClient() const noexcept {
            return m_isMasterClient;
        }

        void SetMasterClient(const bool isMasterClient_) {
            m_isMasterClient = isMasterClient_;
        }
    };
}

namespace EventCode::Game {
    constexpr nByte putCell = 1;
    constexpr nByte replay = 2;
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
            getData().SetMasterClient(true);
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
            getData().SetMasterClient(false);
            // この下はゲームシーンに進んだり対戦相手が設定したりする処理を書きます。
            changeScene(Common::Scene::Game);  // ゲームシーンに進む
        }

        void JoinRoomEventAction(int playerNr, const ExitGames::Common::JVector<int>& playernrs, const ExitGames::LoadBalancing::Player& player) override {
            // 部屋に入室したのが自分の場合、早期リターン
            if (GetClient().getLocalPlayer().getNumber() == player.getNumber()) {
                return;
            }

            s3d::Print(U"対戦相手が入室しました。");
            // この下はゲームシーンに進んだり設定したりする処理を書きます。
            changeScene(Common::Scene::Game);  // ゲームシーンに進む
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

    class Game : public MyScene::Scene {
    private:
        /// <summary>
        /// マスの状態
        /// </summary>
        enum class [[nodiscard]] CellState {
            None,   // 未入力
            Maru,   // 丸
            Batsu   // バツ
        };
        
    private:
        class Cell {
            s3d::RectF m_cell;

            CellState m_state;

            s3d::Point m_coodinate;

        public:
            Cell() noexcept = default;

            constexpr Cell(const double xPos_, const double yPos_, const double size_, const s3d::Point& coodinate_) noexcept
                : m_cell(xPos_, yPos_, size_)
                , m_state(CellState::None)
                , m_coodinate(coodinate_) {}

            [[nodiscard]] bool IsMouseOver() const {
                return m_cell.mouseOver();
            }

            [[nodiscard]] bool IsLeftClicked() const {
                return m_cell.leftClicked();
            }

            [[nodiscard]] constexpr s3d::Vec2 GetCenterPos() const noexcept {
                return m_cell.center();
            }

            [[nodiscard]] constexpr s3d::Point GetCoodinate() const noexcept {
                return m_coodinate;
            }

            CellState GetState() const noexcept {
                return m_state;
            }
            void ChangeState(const CellState& state_) {
                m_state = state_;
            }

            [[nodiscard]] bool IsStateNone() const noexcept {
                return m_state == CellState::None;
            }

            Cell Draw(const s3d::ColorF& color_) const {
                m_cell.draw(color_);
                return *this;
            }

            Cell DrawFrame(const double thickness_, const s3d::ColorF& color_) const {
                m_cell.drawFrame(thickness_, color_);
                return *this;
            }

            void DrawPattern(const double size_, const double thickness_) const {
                switch (m_state) {
                case CellState::Maru:
                    s3d::Circle(m_cell.center(), size_).drawFrame(thickness_, s3d::Palette::Black);
                    break;
                case CellState::Batsu:
                    s3d::Shape2D::Cross(size_, thickness_, m_cell.center()).draw(s3d::Palette::Black);
                    break;
                default:
                    break;
                }
            }
        };


        /// <summary>
        /// 手番
        /// </summary>
        enum class [[nodiscard]] Turn {
            Self,  // 先攻
            Enemy, // 後攻
            Result
        } m_turn;

        // 1マスのサイズ
        static constexpr int32 masuSize = 150;

        // マス目を表示する際の場所の指標
        const s3d::RectF m_ban;

        // masuArrayを2次元配列に変換
        s3d::Grid<Cell> m_cells; // (3, 3, masuArray);

        CellState m_myCellType;


        bool CheckLine(const Cell& state1_, const Cell& state2_, const Cell& state3_) const noexcept {
            if (state1_.IsStateNone() || state2_.IsStateNone() || state3_.IsStateNone()) {
                return false;
            }

            return (state1_.GetState() == state2_.GetState() && state2_.GetState() == state3_.GetState());
        }

        void CheckGameEnd() {
            if (std::all_of(m_cells.begin(), m_cells.end(), [](const Cell& cell_) {return !cell_.IsStateNone(); })) {
                m_turn = Turn::Result;
                return;
            }

            // 縦3行を調べる
            // trueが返ったら赤線を引く
            for (int y : s3d::step(3)) {
                if (CheckLine(m_cells[y][0], m_cells[y][1], m_cells[y][2])) {
                    m_turn = Turn::Result;
                    return;
                }
            }

            // 横3列を調べる
            // trueが返ったら赤線を引く
            for (int x : s3d::step(3)) {
                if (CheckLine(m_cells[0][x], m_cells[1][x], m_cells[2][x])) {
                    m_turn = Turn::Result;
                    return;
                }
            }

            // 斜め(左上から右下)
            // trueが返ったら赤線を引く
            if (CheckLine(m_cells[0][0], m_cells[1][1], m_cells[2][2])) {
                m_turn = Turn::Result;
                return;
            }

            // 斜め(右上から左下)
            // trueが返ったら赤線を引く
            if (CheckLine(m_cells[2][0], m_cells[1][1], m_cells[0][2])) {
                m_turn = Turn::Result;
                return;
            }
        }

        /// <summary>
        /// 相手に置いたマス目の情報を送る
        /// </summary>
        /// <param name="cell_">置いたマス目</param>
        void SendOpponent(const Cell& cell_) {
            ExitGames::Common::Dictionary<nByte, int32> dic;
            dic.put(1, cell_.GetCoodinate().x);
            dic.put(2, cell_.GetCoodinate().y);
            dic.put(3, static_cast<int32>(cell_.GetState()));

            GetClient().opRaiseEvent(true, dic, EventCode::Game::putCell);
        }

        /// <summary>
        /// 送られてきたイベントの制御
        /// </summary>
        void CustomEventAction(int playerNr, nByte eventCode, const ExitGames::Common::Object& eventContent) override {
            switch (eventCode) {
            case EventCode::Game::putCell: {
                auto dic = ExitGames::Common::ValueObject<ExitGames::Common::Dictionary<nByte, int32>>(eventContent).getDataCopy();
                s3d::Point coodinate(*dic.getValue(1), *dic.getValue(2));
                const auto cellType{ static_cast<CellState>(*dic.getValue(3)) };
                EnemyUpdate(coodinate, cellType);
                break;
            }
            case EventCode::Game::replay: {
                auto dic = ExitGames::Common::ValueObject<ExitGames::Common::Dictionary<nByte, bool>>(eventContent).getDataCopy();
                // マスの情報の配列を全て「None」に変更する
                for (auto& cell : m_cells) {
                    cell.ChangeState(CellState::None);
                }
                // 手番を戻す
                m_turn = Turn::Enemy;
            }
            default:
                break;
            }
        }

        void SelfUpdate() {
            // マスをクリックしたら手番によって変更する
            for (auto& cell : m_cells) {
                // マスの状態が「None」ではなかったら早期リターン
                // for文の中の処理なのでreturnではなくcontinue
                if (!cell.IsStateNone()) {
                    continue;
                }

                // マスをクリックしたら、状態を変更する
                // 手番も変更する
                if (cell.IsLeftClicked()) {
                    cell.ChangeState(m_myCellType);
                    SendOpponent(cell);
                    m_turn = Turn::Enemy;
                    CheckGameEnd();
                    return;
                }
            }

        }

        void EnemyUpdate(const s3d::Point& coodinate_, const CellState& cellType) {
            m_cells[coodinate_].ChangeState(cellType);
            m_turn = Turn::Self;
            CheckGameEnd();
            return;
        }

        void Result() {
            if (!getData().IsMasterClient()) {
                return;
            }
            // ボタンを押したか
            if (s3d::SimpleGUI::Button(U"Retry", s3d::Scene::CenterF().movedBy(-100, 250), 200)) {
                ExitGames::Common::Dictionary<nByte, bool> dic;
                dic.put(1, true);
                GetClient().opRaiseEvent(true, dic, EventCode::Game::replay);

                // マスの情報の配列を全て「None」に変更する
                for (auto& cell : m_cells) {
                    cell.ChangeState(CellState::None);
                }
                // 手番を先攻に戻す
                m_turn = Turn::Self;
            }
        }

    public:
        Game(const InitData& init_)
            : IScene(init_)
            , m_turn(getData().IsMasterClient() ? Turn::Self : Turn::Enemy)
            , m_myCellType(m_turn == Turn::Self ? CellState::Maru : CellState::Batsu)
            , m_ban(s3d::Arg::center(s3d::Scene::Center()), masuSize * 3) {
            s3d::Array<Cell> cells;

            for (int y : s3d::step(3)) {
                for (int x : s3d::step(3)) {
                    cells.emplace_back(m_ban.tl().x + x * masuSize, m_ban.tl().y + y * masuSize, masuSize, s3d::Point(x, y));
                }
            }

            m_cells = s3d::Grid<Cell>(3, 3, cells);
        }

        void update() override {
            switch (m_turn)
            {
            case Turn::Self:
                SelfUpdate();
                break;
            case Turn::Enemy:
                break;
            case Turn::Result:
                Result();
                break;
            default:
                break;
            }
        }

        void draw() const override {
            for (const auto& cell : m_cells) {
                cell.Draw(s3d::Palette::White).DrawFrame(5, s3d::Palette::Black);
                if (cell.IsMouseOver()) {
                    cell.Draw(s3d::ColorF(s3d::Palette::Yellow, 0.5));
                }

                cell.DrawPattern(masuSize * 0.4, 5);
            }

            // 縦3行を調べる
            // trueが返ったら赤線を引く
            for (int y : s3d::step(3)) {
                if (CheckLine(m_cells[y][0], m_cells[y][1], m_cells[y][2])) {
                    s3d::Line(m_cells[y][0].GetCenterPos(), m_cells[y][2].GetCenterPos())
                        .stretched(30).draw(4, s3d::ColorF(s3d::Palette::Red, 0.6));
                }
            }

            // 横3列を調べる
            // trueが返ったら赤線を引く
            for (int x : s3d::step(3)) {
                if (CheckLine(m_cells[0][x], m_cells[1][x], m_cells[2][x])) {
                    s3d::Line(m_cells[0][x].GetCenterPos(), m_cells[2][x].GetCenterPos())
                        .stretched(30).draw(4, s3d::ColorF(s3d::Palette::Red, 0.6));
                }
            }

            // 斜め(左上から右下)
            // trueが返ったら赤線を引く
            if (CheckLine(m_cells[0][0], m_cells[1][1], m_cells[2][2])) {
                s3d::Line(m_cells[0][0].GetCenterPos(), m_cells[2][2].GetCenterPos())
                    .stretched(30).draw(4, s3d::ColorF(s3d::Palette::Red, 0.6));
            }

            // 斜め(右上から左下)
            // trueが返ったら赤線を引く
            if (CheckLine(m_cells[2][0], m_cells[1][1], m_cells[0][2])) {
                s3d::Line(m_cells[2][0].GetCenterPos(), m_cells[0][2].GetCenterPos())
                    .stretched(30).draw(4, s3d::ColorF(s3d::Palette::Red, 0.6));
            }
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
    MyScene manager(L"/*ここを自分のappIDに変更してください*/", L"1.0");

    manager.add<Sample::Title>(Common::Scene::Title)
        .add<Sample::Match>(Common::Scene::Match)
        .add<Sample::Game>(Common::Scene::Game)
        .setFadeColor(s3d::ColorF(1.0));

    while (s3d::System::Update()) {
        if (!manager.update()) {
            break;
        }
    }
}
