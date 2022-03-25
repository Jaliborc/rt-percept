#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include <chrono>

using namespace Falcor;

class Evaluator : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<Evaluator>;
    static SharedPtr create(RenderContext* context = nullptr, const Dictionary& dict = {}) {return SharedPtr(new Evaluator);}
    static const char* desc;

    virtual RenderPassReflection reflect(const CompileData& data) override;
    virtual void compile(RenderContext* context, const CompileData& data) override {};
    virtual void execute(RenderContext* context, const RenderData& data) override;
    virtual void setScene(RenderContext* context, const Scene::SharedPtr& scene) override;
    virtual std::string getDesc() override { return desc; }
    virtual void renderUI(Gui::Widgets& widget) override;

private:
    enum class State { Init, Record, Dump, Ready, Invalid };

    void showCam();
    void loadCamFile();

    std::queue<glm::mat4> configs;
    std::queue<glm::mat4> dump_configs;

    int rep = 0;
    int toRecord = 0;
    int frame = 0;
    int cycles = 50;
    bool dumping = false, chrono = false, odd = false;

    double accum = 0;
    double accum2 = 0;
    double accum3 = 0;
    double accum4 = 0;

    Scene::SharedPtr scene;
    std::chrono::time_point<std::chrono::system_clock> before;
    std::chrono::time_point<std::chrono::system_clock> before2;
    State state = State::Invalid;

    Evaluator() {};
};
