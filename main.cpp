#include <SDL.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

constexpr int initial_width = 600;
constexpr int initial_height = 600;
constexpr double pi = 3.14159265358979323846;
constexpr SDL_Color background{20, 70, 80, 255};

double radians(double degrees) {
    return degrees * pi / 180.0;
}

void check(int result) {
    if (result < 0) {
        throw std::runtime_error(SDL_GetError());
    }
}

struct Sdl {
    Sdl() { check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)); }
    ~Sdl() { SDL_Quit(); }
    Sdl(const Sdl&) = delete;
    Sdl& operator=(const Sdl&) = delete;
};

using Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using Texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

Texture make_target(SDL_Renderer* renderer, int width, int height) {
    Texture texture(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_TARGET, width, height),
                    SDL_DestroyTexture);
    if (!texture) {
        throw std::runtime_error(SDL_GetError());
    }
    return texture;
}

void set_color(SDL_Renderer* renderer, SDL_Color color) {
    check(SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a));
}

class Random {
public:
    double uniform(double low, double high) {
        return std::uniform_real_distribution<double>(low, high)(engine_);
    }

    int integer(int low, int high) {
        return std::uniform_int_distribution<int>(low, high)(engine_);
    }

private:
    std::mt19937 engine_{std::random_device{}()};
};

struct Point {
    double x;
    double y;
};

class WaterMandalaPart {
public:
    WaterMandalaPart(int width, int height, double held_space, Random& random)
        : width_(width), height_(height), held_space_(held_space) {
        reset(random, true);
    }

    void update(double dt, Random& random) {
        if (y_ >= height_ / 1.5) {
            reset(random, false);
        } else {
            y_ += speed_ * dt;
        }
    }

    void flow_points(double amplitude, const std::array<double, 3>& factors) {
        points.resize(static_cast<std::size_t>(std::max(2, static_cast<int>(length_))));
        for (std::size_t i = 0; i < points.size(); ++i) {
            const double offset = static_cast<double>(i);
            const double x = x_
                + amplitude * std::cos(radians(y_ + offset))
                    * std::sin(radians(y_ / factors[0] + offset / factors[0]))
                    * std::cos(radians(offset * 2.0))
                - 50.0 * std::sin(radians(offset / factors[1]));
            points[i] = {x, y_ + offset};
        }
    }

    SDL_Color color{};
    std::vector<Point> points;

private:
    void reset(Random& random, bool random_y) {
        x_ = random.uniform(-held_space_, held_space_);
        length_ = random.uniform(width_ / 100.0, width_ / 20.0);
        y_ = random_y ? random.uniform(-height_ / 1.5, height_ / 1.5)
                      : -height_ / 1.5 - length_;
        speed_ = random.uniform(0.06, 12.0);
        color = {static_cast<Uint8>(random.integer(200, 250)),
                 static_cast<Uint8>(random.integer(240, 255)), 255,
                 static_cast<Uint8>(random.integer(2, 30))};
    }

    int width_;
    int height_;
    double held_space_;
    double x_{};
    double y_{};
    double length_{};
    double speed_{};
};

class WaterMandalaEffect {
public:
    explicit WaterMandalaEffect(SDL_Renderer* renderer)
        : renderer_(renderer) {
        sync_output_size();
    }

    bool sync_output_size(bool recreate = false) {
        // Query the window backbuffer, not the currently selected texture.
        check(SDL_SetRenderTarget(renderer_, nullptr));
        int width = 0;
        int height = 0;
        check(SDL_GetRendererOutputSize(renderer_, &width, &height));
        if (width <= 0 || height <= 0) {
            return false;
        }
        if (!recreate && width == width_ && height == height_) {
            return true;
        }
        auto trail = make_target(renderer_, width, height);
        auto lines = make_target(renderer_, width, height);
        check(SDL_SetTextureBlendMode(trail.get(), SDL_BLENDMODE_NONE));
        check(SDL_SetTextureBlendMode(lines.get(), SDL_BLENDMODE_BLEND));
        trail_ = std::move(trail);
        lines_ = std::move(lines);
        width_ = width;
        height_ = height;
        randomize(false);
        return true;
    }

    void randomize(bool clicked) {
        const double held_space = random_.uniform(width_ / 20.0, width_ / 10.0);
        const int density = random_.integer(200, clicked ? 500 : 400);
        amplitude_ = random_.uniform(width_ / 5.0, width_ / 3.0);
        for (double& factor : factors_) {
            factor = random_.uniform(3.0, 10.0);
        }
        symmetry_ = random_.integer(7, 12);
        parts_.clear();
        parts_.reserve(static_cast<std::size_t>(density));
        for (int i = 0; i < density; ++i) {
            parts_.emplace_back(width_, height_, held_space, random_);
        }
        check(SDL_SetRenderTarget(renderer_, trail_.get()));
        set_color(renderer_, background);
        check(SDL_RenderClear(renderer_));
        check(SDL_SetRenderTarget(renderer_, nullptr));
    }

    void update(double dt) {
        frame_ += 156.0 * dt;
        for (auto& part : parts_) {
            part.update(dt, random_);
            // All rotated copies share the same underlying curve.
            part.flow_points(amplitude_, factors_);
        }
    }

    void draw() {
        check(SDL_SetRenderTarget(renderer_, trail_.get()));
        check(SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND));
        set_color(renderer_, {background.r, background.g, background.b, 10});
        check(SDL_RenderFillRect(renderer_, nullptr));

        check(SDL_SetRenderTarget(renderer_, lines_.get()));
        set_color(renderer_, {0, 0, 0, 0});
        check(SDL_RenderClear(renderer_));
        // Pygame's draw.lines replaces pixels on its transparent surface.
        // Blend only when compositing the completed line layer onto the trail.
        check(SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE));
        for (int k = 0; k < symmetry_; ++k) {
            const double rotation = radians(-frame_ / 10.0 + (180.0 / symmetry_) * k);
            const double cos_r = std::cos(rotation);
            const double sin_r = std::sin(rotation);
            for (const auto& part : parts_) {
                rotated_.resize(part.points.size());
                for (std::size_t i = 0; i < part.points.size(); ++i) {
                    const auto& point = part.points[i];
                    rotated_[i] = {
                        static_cast<int>(width_ / 2.0 + point.x * cos_r - point.y * sin_r),
                        static_cast<int>(height_ / 2.0 + point.x * sin_r + point.y * cos_r)
                    };
                }
                set_color(renderer_, part.color);
                check(SDL_RenderDrawLines(renderer_, rotated_.data(),
                                          static_cast<int>(rotated_.size())));
            }
        }

        check(SDL_SetRenderTarget(renderer_, trail_.get()));
        check(SDL_RenderCopy(renderer_, lines_.get(), nullptr, nullptr));
        check(SDL_SetRenderTarget(renderer_, nullptr));
        // Explicit pixel dimensions: never stretch an old-sized image to the window.
        const SDL_Rect destination{0, 0, width_, height_};
        check(SDL_RenderCopy(renderer_, trail_.get(), nullptr, &destination));
        SDL_RenderPresent(renderer_);
    }

private:
    SDL_Renderer* renderer_;
    Texture trail_{nullptr, SDL_DestroyTexture};
    Texture lines_{nullptr, SDL_DestroyTexture};
    int width_ = 0;
    int height_ = 0;
    Random random_;
    std::vector<WaterMandalaPart> parts_;
    std::vector<SDL_Point> rotated_;
    std::array<double, 3> factors_{};
    double amplitude_{};
    double frame_{};
    int symmetry_{};
};

struct Options {
    int frames = 0;
    bool help = false;
};

Options parse_options(int argc, char* argv[]) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--help" || argument == "-h") {
            options.help = true;
        } else if (argument == "--frames" && i + 1 < argc) {
            const std::string_view value(argv[++i]);
            const auto result = std::from_chars(value.data(), value.data() + value.size(),
                                                options.frames);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size()
                || options.frames <= 0) {
                throw std::runtime_error("--frames requires a positive integer");
            }
        } else {
            throw std::runtime_error("Unknown or incomplete argument: " + std::string(argument));
        }
    }
    return options;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const Options options = parse_options(argc, argv);
        if (options.help) {
            std::cout << "Usage: wave-effect [--frames N]\n"
                         "Click to randomize the pattern. Press Escape to quit.\n"
                         "F11 toggles fullscreen at the display's pixel resolution.\n"
                         "--frames N: exit after N frames (for smoke testing).\n";
            return 0;
        }

        Sdl sdl;
        Window window(SDL_CreateWindow("Water Mandala", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, initial_width, initial_height,
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI),
                      SDL_DestroyWindow);
        if (!window) {
            throw std::runtime_error(SDL_GetError());
        }
        Renderer renderer(SDL_CreateRenderer(window.get(), -1,
                          SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE),
                          SDL_DestroyRenderer);
        if (!renderer) {
            renderer.reset(SDL_CreateRenderer(window.get(), -1,
                           SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE));
        }
        if (!renderer) {
            throw std::runtime_error(SDL_GetError());
        }

        WaterMandalaEffect effect(renderer.get());
        using Clock = std::chrono::steady_clock;
        const auto frame_duration = std::chrono::duration_cast<Clock::duration>(
            std::chrono::duration<double>(1.0 / 60.0));
        auto previous = Clock::now();
        bool running = true;
        int frames_left = options.frames;
        while (running) {
            std::this_thread::sleep_until(previous + frame_duration);
            const auto now = Clock::now();
            const double dt = std::chrono::duration<double>(now - previous).count();
            previous = now;

            SDL_Event event;
            bool recreate_targets = false;
            bool randomize_requested = false;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT
                    || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11
                           && !event.key.repeat) {
                    const bool fullscreen = (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_FULLSCREEN) != 0;
                    check(SDL_SetWindowFullscreen(window.get(), fullscreen ? 0U
                          : static_cast<Uint32>(SDL_WINDOW_FULLSCREEN_DESKTOP)));
                    recreate_targets = true;
                } else if (event.type == SDL_RENDER_TARGETS_RESET || event.type == SDL_RENDER_DEVICE_RESET) {
                    recreate_targets = true;
                } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    randomize_requested = true;
                }
            }
            if (!running) {
                break;
            }
            // Fullscreen transitions and monitor/DPI changes can settle asynchronously.
            // Check actual drawable pixels each frame; recreate only when needed.
            if (!effect.sync_output_size(recreate_targets)) {
                continue;
            }
            if (randomize_requested) {
                effect.randomize(true);
            }
            effect.update(dt);
            effect.draw();
            if (options.frames > 0 && --frames_left == 0) {
                break;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "wave-effect: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
