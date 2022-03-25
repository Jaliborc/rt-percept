#include "Evaluator.h"

const char* Evaluator::desc = "Moving camera performance evaluation.";
extern "C" __declspec(dllexport) const char* getProjDir() { return PROJECT_DIR; }
extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary & lib)
{
    lib.registerClass("Evaluator", Evaluator::desc, Evaluator::create);
}

RenderPassReflection Evaluator::reflect(const CompileData& compileData)
{
    RenderPassReflection reflector;
    reflector.addInputOutput("passThrough", "Color passthrough. Needed because render passes without outputs are pruned").format(ResourceFormat::RGBA32Float);
    return reflector;
}

void Evaluator::setScene(RenderContext* context, const Scene::SharedPtr& scene)
{
    this->scene = scene;
}

void Evaluator::execute(RenderContext* context, const RenderData& data)
{
    using namespace std::chrono_literals;
    if (state == State::Init)
    {
        std::cout << "Moving camera render time:" << std::endl;

        rep = 0;
        frame = 0;
        accum = 0;
        accum2 = 0;
        accum3 = 0;
        accum4 = 0;
        if (dumping)
        {
            state = State::Dump;
            scene->getCamera()->queueConfigs(dump_configs);
        }
        else
        {
            state = State::Record;
            scene->getCamera()->queueConfigs(configs);
        }
    }
    else if (state == State::Record)
    {
        //std::this_thread::sleep_for(1s);
        if (rep % 2 == 1)
        {
            accum2 += gpFramework->getFrameRate().getLastFrameTime();
            accum4 += (std::chrono::system_clock::now() - before2).count();
            if (rep == 1 && frame != 0) // we finished a frame last round
            {
                if (odd)
                  if (chrono)
                    std::cout << accum4 / cycles << std::endl;
                  else
                    std::cout << accum2 / cycles << std::endl;

                accum2 = 0;
                accum4 = 0;
            }
            before = std::chrono::system_clock::now();
        }
        if (rep % 2 == 0)
        {
            before2 = std::chrono::system_clock::now();
            if (rep != 0 || frame != 0)
            {
                accum += gpFramework->getFrameRate().getLastFrameTime();
                accum3 += (std::chrono::system_clock::now() - before).count();
                if (rep == 0) // we finished a frame last round
                {
                  if (!odd)
                    if (chrono)
                      std::cout << accum3 / cycles << std::endl;
                    else
                      std::cout << accum / cycles << std::endl;

                  accum = 0;
                  accum3 = 0;
                }
            }
        }
        if (frame == toRecord)
        {
            state = State::Ready;
        }
        if (++rep == cycles * 2)
        {
            rep = 0;
            frame++;
        }
    }
    else if (state == State::Dump)
    {
        if (frame % 2 == 1)
        {
            auto ref = data["passThrough"]->asTexture();
            Texture::SharedPtr dest = Texture::create2D(ref->getWidth(), ref->getHeight(), Falcor::ResourceFormat::RGBA8UnormSrgb, 1, 1, (const void*)nullptr, Falcor::ResourceBindFlags::RenderTarget);
            context->blit(ref->getSRV(), dest->getRTV());
            dest->asTexture()->captureToFile(0, 0, std::to_string(frame / 2) + ".png");
        }
        if (frame == 2 * toRecord)
        {
            state = State::Ready;
        }
        frame++;
    }
}

void Evaluator::loadCamFile() {

    std::string filename;
    FileDialogFilterVec filters = { {"txt"} };

    if ((state == State::Invalid || state == State::Ready) && openFileDialog(filters, filename))
    {
        configs = std::queue<glm::mat4>();
        dump_configs = std::queue<glm::mat4>();
        std::ifstream infile(filename);
        std::string line;
        while (std::getline(infile, line))
        {
            if (line.length() != 0)
            {
                glm::mat4 mat1, mat2;
                std::stringstream ss(line);
                for (int i = 0; i < 4; i++)
                {
                    ss >> mat1[i][0] >> mat1[i][1] >> mat1[i][2] >> mat1[i][3];
                    std::getline(infile, line);
                    ss = std::stringstream(line);
                }
                mat1 = glm::inverse(mat1);
                mat1[2] = -mat1[2];
                for (int i = 0; i < 4; i++)
                {
                    ss >> mat2[i][0] >> mat2[i][1] >> mat2[i][2] >> mat2[i][3];
                    std::getline(infile, line);
                    ss = std::stringstream(line);
                }
                mat2 = glm::inverse(mat2);
                mat2[2] = -mat2[2];
                for (int i = 0; i < cycles; i++)
                {
                    configs.push(mat1);
                    configs.push(mat2);
                }
                dump_configs.push(mat1);
                dump_configs.push(mat2);
                toRecord++;
            }
        }
        state = State::Ready;
    }
}

void Evaluator::showCam()
{
    glm::mat4 mat = scene->getCamera()->getViewMatrix();
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            std::cout << mat[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void Evaluator::renderUI(Gui::Widgets& widget)
{
    if (widget.button("Dump Camera Proprieties"))
        showCam();
    if (widget.button("Load Viewpoints File"))
        loadCamFile();

    widget.var<int>("Repetitions", cycles, 1);
    widget.checkbox("Dump Renderings", dumping);
    widget.checkbox("Use <chrono>", chrono);
    widget.checkbox("Measure Odd", odd);

    if (state == State::Ready && widget.button("Go!"))
        state = State::Init;
}
