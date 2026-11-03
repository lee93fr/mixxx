#include "mixer/performancepadcontrol.h"

#include "control/controlproxy.h"
#include "test/mockedenginebackendtest.h"

class PerformancePadControlTest : public MockedEngineBackendTest {
  protected:
    double get(const char* key) {
        return ControlObject::get(ConfigKey(m_sGroup1, key));
    }
    void set(const char* key, double value) {
        ControlObject::set(ConfigKey(m_sGroup1, key), value);
    }
    void load(double bpm = 120) {
        m_pMixerDeck1->loadFakeTrack(false, bpm);
        ProcessBuffer();
        QApplication::processEvents();
    }
    void mode(PerformancePadControl::Mode mode) {
        set("pad_mode", mode);
    }
};

TEST_F(PerformancePadControlTest, StartsInHotcueAndExposesUnscaledMode) {
    EXPECT_EQ(get("pad_mode"), PerformancePadControl::Hotcue);
    ControlProxy modeControl(m_sGroup1, "pad_mode");
    for (int mode : {0, 1, 2, 4, 5, 6}) {
        const auto key = QStringLiteral("pad_mode_%1").arg(mode);
        ControlObject::set(ConfigKey(m_sGroup1, key), 1);
        ControlObject::set(ConfigKey(m_sGroup1, key), 0);
        EXPECT_EQ(modeControl.get(), mode);
        EXPECT_EQ(modeControl.getParameter(), mode);
    }
}

TEST_F(PerformancePadControlTest, MissingGridDisablesSlicer) {
    load(0);
    mode(PerformancePadControl::Slicer);
    EXPECT_EQ(get("slicer_ready"), 0);
    set("performance_pad_1", 1);
    EXPECT_EQ(get("loop_enabled"), 0);
    EXPECT_EQ(get("slip_enabled"), 0);
    mode(PerformancePadControl::Hotcue);
    EXPECT_EQ(get("performance_pad_1"), 0);
}

TEST_F(PerformancePadControlTest, ModeButtonsChangePadActions) {
    load();
    set("pad_mode_2", 1);
    set("pad_mode_2", 0);
    ASSERT_EQ(get("slicer_ready"), 1);
    set("performance_pad_3", 1);
    EXPECT_EQ(get("loop_start_position"), 88200);
    EXPECT_EQ(get("loop_end_position"), 132300);
    EXPECT_EQ(get("slip_enabled"), 1);
    set("pad_mode_0", 1);
    set("pad_mode_0", 0);
    EXPECT_EQ(get("performance_pad_3"), 0);
    EXPECT_EQ(get("slip_enabled"), 0);
    set("performance_pad_3", 1);
    set("performance_pad_3", 0);
    EXPECT_GT(get("hotcue_3_status"), 0);
}

TEST_F(PerformancePadControlTest, HotcueCreatesAndClearsCue) {
    load();
    set("performance_pad_1", 1);
    set("performance_pad_1", 0);
    EXPECT_GT(get("hotcue_1_status"), 0);
    EXPECT_EQ(get("performance_pad_1_active"), 1);
    set("performance_pad_1_clear", 1);
    set("performance_pad_1_clear", 0);
    EXPECT_EQ(get("hotcue_1_status"), 0);
    EXPECT_EQ(get("performance_pad_1_active"), 0);
}

TEST_F(PerformancePadControlTest, SlicerRestoresLoopAndSlipAfterModeChange) {
    load();
    set("loop_start_position", 44100);
    set("loop_end_position", 88200);
    set("loop_enabled", 1);
    mode(PerformancePadControl::Slicer);
    ASSERT_EQ(get("slicer_ready"), 1);
    set("performance_pad_3", 1);
    EXPECT_EQ(get("loop_start_position"), 88200);
    EXPECT_EQ(get("loop_end_position"), 132300);
    EXPECT_EQ(get("slip_enabled"), 1);
    mode(PerformancePadControl::Hotcue);
    EXPECT_EQ(get("performance_pad_3"), 0);
    EXPECT_EQ(get("loop_start_position"), 44100);
    EXPECT_EQ(get("loop_end_position"), 88200);
    EXPECT_EQ(get("loop_enabled"), 1);
    EXPECT_EQ(get("slip_enabled"), 0);
}

TEST_F(PerformancePadControlTest, SlicerReturnsToLastHeldSliceThenReleases) {
    load();
    mode(PerformancePadControl::Slicer);
    ASSERT_EQ(get("slicer_ready"), 1);
    set("performance_pad_2", 1);
    set("performance_pad_6", 1);
    EXPECT_EQ(get("loop_start_position"), 220500);
    set("performance_pad_6", 0);
    EXPECT_EQ(get("loop_start_position"), 44100);
    set("performance_pad_2", 0);
    EXPECT_EQ(get("loop_enabled"), 0);
    EXPECT_EQ(get("slip_enabled"), 0);
}

TEST_F(PerformancePadControlTest, SlicerLoopRestoresDomainThenOriginalLoop) {
    load();
    set("loop_start_position", 44100);
    set("loop_end_position", 88200);
    set("loop_enabled", 1);
    set("slip_enabled", 1);
    mode(PerformancePadControl::SlicerLoop);
    ASSERT_EQ(get("slicer_ready"), 1);
    EXPECT_EQ(get("loop_start_position"), 0);
    EXPECT_EQ(get("loop_end_position"), 352800);
    set("performance_pad_8", 1);
    EXPECT_EQ(get("loop_start_position"), 308700);
    set("performance_pad_8", 0);
    EXPECT_EQ(get("loop_end_position"), 352800);
    EXPECT_EQ(get("loop_enabled"), 1);
    EXPECT_EQ(get("slip_enabled"), 1);
    mode(PerformancePadControl::Hotcue);
    EXPECT_EQ(get("loop_start_position"), 44100);
    EXPECT_EQ(get("loop_end_position"), 88200);
    EXPECT_EQ(get("loop_enabled"), 1);
}

TEST_F(PerformancePadControlTest, TrackChangeReleasesSlice) {
    load();
    mode(PerformancePadControl::Slicer);
    set("performance_pad_4", 1);
    ASSERT_EQ(get("slip_enabled"), 1);
    load(0);
    EXPECT_EQ(get("performance_pad_4"), 0);
    EXPECT_EQ(get("slip_enabled"), 0);
    EXPECT_EQ(get("slicer_ready"), 0);
}

TEST_F(PerformancePadControlTest, RollReleasesAndPreservesExistingSlip) {
    load();
    for (int slip : {0, 1}) {
        set("slip_enabled", slip);
        mode(PerformancePadControl::Roll);
        set("performance_pad_4", 1);
        EXPECT_EQ(get("loop_enabled"), 1);
        EXPECT_EQ(get("slip_enabled"), 1);
        mode(PerformancePadControl::Hotcue);
        EXPECT_EQ(get("loop_enabled"), 0);
        EXPECT_EQ(get("slip_enabled"), slip);
    }
}

TEST_F(PerformancePadControlTest, SlipResumesUninterruptedTimelineAfterSlice) {
    load();
    set("play", 1);
    for (int i = 0; i < 4; ++i) {
        ProcessBuffer();
    }
    auto* buffer = m_pChannel1->getEngineBuffer();
    const double before = buffer->getExactPlayPos().value();
    ProcessBuffer();
    const double advance = buffer->getExactPlayPos().value() - before;
    ASSERT_GT(advance, 0);
    const double start = buffer->getExactPlayPos().value();
    mode(PerformancePadControl::Slicer);
    set("performance_pad_7", 1);
    for (int i = 0; i < 10; ++i) {
        ProcessBuffer();
    }
    EXPECT_GT(buffer->getExactPlayPos().value(), 6 * 22050);
    set("performance_pad_7", 0);
    ProcessBuffer();
    EXPECT_NEAR(buffer->getExactPlayPos().value(), start + 11 * advance, 2);
}

TEST_F(PerformancePadControlTest, MultipleRollsRestorePreviousHoldAndLoop) {
    load();
    set("loop_start_position", 44100);
    set("loop_end_position", 88200);
    set("loop_enabled", 1);
    mode(PerformancePadControl::Roll);
    set("performance_pad_4", 1);
    set("performance_pad_2", 1);
    EXPECT_NEAR(get("loop_end_position") - get("loop_start_position"), 11025, 1);
    set("performance_pad_2", 0);
    EXPECT_NEAR(get("loop_end_position") - get("loop_start_position"), 44100, 1);
    set("performance_pad_4", 0);
    EXPECT_EQ(get("loop_start_position"), 44100);
    EXPECT_EQ(get("loop_end_position"), 88200);
    EXPECT_EQ(get("loop_enabled"), 1);
    EXPECT_EQ(get("slip_enabled"), 0);
}

TEST_F(PerformancePadControlTest, LoopPadsToggleSelectedLength) {
    load();
    mode(PerformancePadControl::Loop);
    set("performance_pad_4", 1);
    set("performance_pad_4", 0);
    EXPECT_EQ(get("loop_enabled"), 1);
    EXPECT_EQ(get("loop_end_position") - get("loop_start_position"), 88200);
    EXPECT_EQ(get("performance_pad_4_active"), 1);
    set("performance_pad_4", 1);
    set("performance_pad_4", 0);
    EXPECT_EQ(get("loop_enabled"), 0);
    EXPECT_EQ(get("performance_pad_4_active"), 0);
}

TEST_F(PerformancePadControlTest, BeatJumpPadsMoveInBothDirections) {
    load();
    set("playposition", 0.4);
    ProcessBuffer();
    const double before = m_pChannel1->getEngineBuffer()->getExactPlayPos().value();
    mode(PerformancePadControl::BeatJump);
    set("performance_pad_2", 1);
    set("performance_pad_2", 0);
    ProcessBuffer();
    EXPECT_NEAR(m_pChannel1->getEngineBuffer()->getExactPlayPos().value(), before + 22050, 2);
    set("performance_pad_3", 1);
    set("performance_pad_3", 0);
    ProcessBuffer();
    EXPECT_NEAR(m_pChannel1->getEngineBuffer()->getExactPlayPos().value(), before - 22050, 2);
}

TEST_F(PerformancePadControlTest, SamplerPadsStartAndStopSharedSlots) {
    std::vector<std::unique_ptr<Sampler>> samplers;
    for (int i = 1; i <= 8; ++i) {
        const auto group = QStringLiteral("[Sampler%1]").arg(i);
        samplers.emplace_back(std::make_unique<Sampler>(nullptr,
                m_pConfig,
                m_pEngineMixer.get(),
                m_pEffectsManager.get(),
                EngineChannel::CENTER,
                m_pEngineMixer->registerChannelGroup(group)));
        samplers.back()->loadFakeTrack(false, 120);
    }
    m_pNumSamplers->set(8);
    ProcessBuffer();
    QApplication::processEvents();
    mode(PerformancePadControl::Sampler);
    for (int i = 1; i <= 8; ++i) {
        const auto padKey = QStringLiteral("performance_pad_%1").arg(i);
        const auto group = QStringLiteral("[Sampler%1]").arg(i);
        ControlObject::set(ConfigKey(m_sGroup1, padKey), 1);
        ControlObject::set(ConfigKey(m_sGroup1, padKey), 0);
        ProcessBuffer();
        EXPECT_EQ(ControlObject::get(ConfigKey(group, "play")), 1);
        ControlObject::set(ConfigKey(m_sGroup1, padKey + "_clear"), 1);
        ControlObject::set(ConfigKey(m_sGroup1, padKey + "_clear"), 0);
        ProcessBuffer();
        EXPECT_EQ(ControlObject::get(ConfigKey(group, "play")), 0);
    }
    mode(PerformancePadControl::Hotcue);
}
