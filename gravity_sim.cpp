/*
 * 3D Solar System Gravity Simulation
 * ====================================
 * Sun + 8 planets + Moon + Asteroid Belt + Saturn's Rings + Halley's Comet
 *
 * Controls:
 *   Left Drag    - Orbit camera
 *   Scroll       - Zoom in/out
 *   Space        - Pause / Resume
 *   R            - Reset
 *   T            - Toggle orbital trails
 *   F            - Follow a planet (cycles through bodies)
 *   ESC          - Quit
 */

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <string>
#include <chrono>

// ─── Constants ───────────────────────────────────────────────────────────────
static const int   WIN_W         = 1280;
static const int   WIN_H         = 720;
static const float G             = 0.0005f;
static const float SOFTENING     = 0.08f;
static const float DT            = 0.0015f;
static const int   TRAIL_LEN     = 180;
static const int   NUM_ASTEROIDS = 180;

// ─── Shaders ─────────────────────────────────────────────────────────────────

// Planet / asteroid point-sprite rendered as a shaded sphere
static const char* PLANET_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec3  aColor;
layout(location = 2) in float aRadius;
layout(location = 3) in float aEmissive;

uniform mat4  uMV;
uniform mat4  uP;
uniform float uViewH;

out vec3  vColor;
out float vEmissive;
out vec3  vViewPos;

void main() {
    vec4 vp     = uMV * vec4(aPos, 1.0);
    gl_Position = uP * vp;
    vViewPos    = vp.xyz;
    float dist  = max(-vp.z, 0.05);
    gl_PointSize = clamp(uViewH * aRadius / (dist * 0.5774), 2.5, 512.0);
    vColor    = aColor;
    vEmissive = aEmissive;
}
)glsl";

static const char* PLANET_FRAG = R"glsl(
#version 330 core
in vec3  vColor;
in float vEmissive;
in vec3  vViewPos;

uniform vec3 uLightView;

out vec4 FragColor;

void main() {
    vec2  coord = gl_PointCoord * 2.0 - 1.0;
    float r2    = dot(coord, coord);
    if (r2 > 1.0) discard;

    // Sun: emissive glow
    if (vEmissive > 0.5) {
        float r      = sqrt(r2);
        float core   = pow(1.0 - r, 1.4);
        float corona = pow(max(1.0 - r * 1.15, 0.0), 4.0);
        vec3  inner  = mix(vec3(1.0,0.60,0.08), vec3(1.0,0.97,0.82), core);
        vec3  outer  = vec3(1.0,0.35,0.05) * corona * 0.7;
        FragColor    = vec4((inner + outer) * (0.7 + core * 0.9), 1.0);
        return;
    }

    // Planet sphere with Phong shading
    float z      = sqrt(1.0 - r2);
    vec3  normal = normalize(vec3(coord.x, -coord.y, z));

    vec3  lightDir = normalize(uLightView - vViewPos);
    float diff     = max(dot(normal, lightDir), 0.0);
    float ambient  = 0.07;

    vec3 color = vColor * (ambient + diff * 0.93);

    // Specular highlight
    vec3  halfV = normalize(lightDir + vec3(0.0, 0.0, 1.0));
    float spec  = pow(max(dot(normal, halfV), 0.0), 55.0) * 0.38;
    color += vec3(spec);

    // Atmosphere rim
    float rim = pow(1.0 - z, 4.0);
    color    += vColor * rim * 0.55;

    // Day/night terminator
    float term = smoothstep(-0.08, 0.15, dot(normal, lightDir));
    color     *= mix(0.03, 1.0, term);

    FragColor = vec4(color, 1.0);
}
)glsl";

// Orbital trails
static const char* TRAIL_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec3  aColor;
layout(location = 2) in float aAge;

uniform mat4 uMVP;
out vec3  vColor;
out float vAge;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
    vAge   = aAge;
}
)glsl";

static const char* TRAIL_FRAG = R"glsl(
#version 330 core
in vec3  vColor;
in float vAge;
out vec4 FragColor;
void main() {
    float alpha = pow(1.0 - vAge, 2.2) * 0.55;
    FragColor   = vec4(vColor * 0.85, alpha);
}
)glsl";

// Saturn rings
static const char* RING_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in float aAlpha;
uniform mat4 uMVP;
out float vAlpha;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vAlpha = aAlpha;
}
)glsl";

static const char* RING_FRAG = R"glsl(
#version 330 core
in  float vAlpha;
out vec4  FragColor;
void main() { FragColor = vec4(0.84, 0.78, 0.62, vAlpha); }
)glsl";

// Reference grid
static const char* GRID_VERT = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)glsl";

static const char* GRID_FRAG = R"glsl(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(0.10, 0.13, 0.20, 0.28); }
)glsl";

// ─── Utilities ───────────────────────────────────────────────────────────────
static GLuint compileShader(GLenum t, const char* src) {
    GLuint s = glCreateShader(t);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char b[512]; glGetShaderInfoLog(s,512,nullptr,b); std::cerr<<"Shader: "<<b<<"\n"; }
    return s;
}
static GLuint linkProg(const char* vs, const char* fs) {
    GLuint v=compileShader(GL_VERTEX_SHADER,vs), f=compileShader(GL_FRAGMENT_SHADER,fs);
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if (!ok) { char b[512]; glGetProgramInfoLog(p,512,nullptr,b); std::cerr<<"Link: "<<b<<"\n"; }
    return p;
}

// ─── Body ────────────────────────────────────────────────────────────────────
struct Body {
    std::string name;
    glm::vec3   pos, vel;
    float       mass;
    float       radius;
    glm::vec3   color;
    bool        isSun      = false;
    bool        hasRing    = false;
    float       ringInner  = 0.f, ringOuter = 0.f;
    bool        showTrail  = true;
    bool        isAsteroid = false;

    std::vector<glm::vec3> trail;
    int trailHead = 0, trailCount = 0;

    void pushTrail(const glm::vec3& p) {
        if ((int)trail.size() < TRAIL_LEN) trail.resize(TRAIL_LEN);
        trail[trailHead] = p;
        trailHead  = (trailHead + 1) % TRAIL_LEN;
        trailCount = std::min(trailCount + 1, TRAIL_LEN);
    }
};

// ─── Simulation ──────────────────────────────────────────────────────────────
class SolarSim {
public:
    std::vector<Body> bodies;
    bool paused     = false;
    bool showTrails = true;

    void init() {
        bodies.clear();

        // Sun
        addB("Sun",     {0,0,0},    {0,0,0},    1000.f, 1.40f, {1.00f,0.88f,0.30f}, true);
        // Planets start on +X axis; velocity applied below
        addB("Mercury", {3.6f,0,0}, {},           0.40f, 0.10f, {0.72f,0.62f,0.52f});
        addB("Venus",   {5.8f,0,0}, {},           0.90f, 0.20f, {0.95f,0.82f,0.52f});
        addB("Earth",   {8.2f,0,0}, {},           1.00f, 0.22f, {0.22f,0.52f,0.98f});
        addB("Moon",    {8.9f,0,0}, {},           0.06f, 0.07f, {0.78f,0.78f,0.78f});
        addB("Mars",    {12.f,0,0}, {},           0.65f, 0.15f, {0.88f,0.38f,0.20f});
        addB("Jupiter", {22.f,0,0}, {},          12.0f,  0.66f, {0.84f,0.68f,0.50f});

        // Saturn with rings
        {
            Body b;
            b.name="Saturn"; b.pos={32.f,0,0}; b.mass=10.f;
            b.radius=0.56f; b.color={0.95f,0.87f,0.65f};
            b.hasRing=true; b.ringInner=0.90f; b.ringOuter=1.80f;
            b.showTrail=true;
            bodies.push_back(b);
        }

        addB("Uranus",  {43.f,0,0}, {},           5.0f, 0.42f, {0.58f,0.88f,0.96f});
        addB("Neptune", {54.f,0,0}, {},           5.0f, 0.40f, {0.24f,0.38f,0.98f});
        addB("Pluto",   {72.f,0,0}, {},           0.20f,0.07f, {0.78f,0.72f,0.65f});

        // Halley's comet (eccentric, inclined)
        {
            Body b;
            b.name="Halley"; b.pos={5.f,2.5f,0.f}; b.mass=0.05f;
            b.radius=0.08f; b.color={0.88f,0.94f,1.00f};
            b.vel={0.f,-0.04f,0.46f}; b.showTrail=true;
            bodies.push_back(b);
        }

        // Assign orbital velocities
        // indices: 0=Sun,1=Mercury,2=Venus,3=Earth,4=Moon,5=Mars,
        //          6=Jupiter,7=Saturn,8=Uranus,9=Neptune,10=Pluto,11=Halley
        bodies[0].vel = {0,0,0};
        bodies[1].vel = inclinedOrbit(bodies[1].pos, bodies[0].pos, bodies[0].mass,  7.0f);
        bodies[2].vel = inclinedOrbit(bodies[2].pos, bodies[0].pos, bodies[0].mass,  3.4f);
        bodies[3].vel = circularOrbit(bodies[3].pos, bodies[0].pos, bodies[0].mass);
        bodies[4].vel = bodies[3].vel
                      + circularOrbit(bodies[4].pos, bodies[3].pos, bodies[3].mass);
        bodies[5].vel = inclinedOrbit(bodies[5].pos, bodies[0].pos, bodies[0].mass,  1.9f);
        bodies[6].vel = inclinedOrbit(bodies[6].pos, bodies[0].pos, bodies[0].mass,  1.3f);
        bodies[7].vel = inclinedOrbit(bodies[7].pos, bodies[0].pos, bodies[0].mass,  2.5f);
        bodies[8].vel = inclinedOrbit(bodies[8].pos, bodies[0].pos, bodies[0].mass,  0.8f);
        bodies[9].vel = inclinedOrbit(bodies[9].pos, bodies[0].pos, bodies[0].mass,  1.8f);
        bodies[10].vel= inclinedOrbit(bodies[10].pos,bodies[0].pos, bodies[0].mass, 17.0f);
        // Halley vel already set

        // Asteroid belt
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> rD(15.5f,20.5f),
            aD(0.f,6.2832f), yD(-0.35f,0.35f), eD(-0.06f,0.06f), gD(0.38f,0.72f);
        for (int i = 0; i < NUM_ASTEROIDS; i++) {
            float r=rD(rng), a=aD(rng), g=gD(rng);
            Body b;
            b.name="Asteroid";
            b.pos={r*std::cos(a), yD(rng), r*std::sin(a)};
            b.mass=0.04f; b.radius=0.04f;
            b.color=glm::vec3(g, g*0.95f, g*0.88f);
            b.isAsteroid=true; b.showTrail=false;
            b.vel = circularOrbit(b.pos, bodies[0].pos, bodies[0].mass);
            b.vel.x += eD(rng); b.vel.z += eD(rng);
            bodies.push_back(b);
        }
    }

    void step() {
        if (paused) return;
        int n = (int)bodies.size();
        std::vector<glm::vec3> acc(n, glm::vec3(0.f));
        for (int i = 0; i < n; i++) {
            for (int j = i+1; j < n; j++) {
                glm::vec3 d  = bodies[j].pos - bodies[i].pos;
                float     d2 = glm::dot(d,d) + SOFTENING*SOFTENING;
                float     f  = G / (d2 * std::sqrt(d2));
                glm::vec3 fv = d * f;
                acc[i] += fv * bodies[j].mass;
                acc[j] -= fv * bodies[i].mass;
            }
        }
        static int tick = 0;
        for (int i = 0; i < n; i++) {
            bodies[i].vel += acc[i] * DT;
            bodies[i].pos += bodies[i].vel * DT;
            if (bodies[i].showTrail && showTrails && tick%4==0)
                bodies[i].pushTrail(bodies[i].pos);
        }
        tick++;
    }

private:
    void addB(const std::string& nm, glm::vec3 p, glm::vec3 v,
              float m, float r, glm::vec3 col, bool sun=false) {
        Body b; b.name=nm; b.pos=p; b.vel=v; b.mass=m;
        b.radius=r; b.color=col; b.isSun=sun; b.showTrail=!sun;
        bodies.push_back(b);
    }

    // Circular orbit in XZ plane
    static glm::vec3 circularOrbit(const glm::vec3& p, const glm::vec3& c, float M) {
        glm::vec3 d = p - c;
        float r = std::sqrt(d.x*d.x + d.z*d.z);
        float v = std::sqrt(G * M / std::max(r, 0.01f));
        return glm::normalize(glm::vec3(-d.z, 0.f, d.x)) * v;
    }

    // Circular orbit with inclination (deg) – tilts velocity upward
    static glm::vec3 inclinedOrbit(const glm::vec3& p, const glm::vec3& c,
                                   float M, float incDeg) {
        glm::vec3 base = circularOrbit(p, c, M);
        float spd  = glm::length(base);
        float inc  = glm::radians(incDeg);
        return glm::vec3(base.x * std::cos(inc),
                         spd   * std::sin(inc),
                         base.z * std::cos(inc));
    }
};

// ─── Camera ──────────────────────────────────────────────────────────────────
struct Camera {
    float yaw=20.f, pitch=28.f, radius=85.f;
    glm::vec3 target={0,0,0};
    glm::mat4 view() const {
        float y=glm::radians(yaw), p=glm::radians(pitch);
        glm::vec3 eye = target + radius*glm::vec3(
            std::cos(p)*std::cos(y), std::sin(p), std::cos(p)*std::sin(y));
        return glm::lookAt(eye, target, {0,1,0});
    }
    glm::mat4 proj(float asp) const {
        return glm::perspective(glm::radians(55.f), asp, 0.05f, 2000.f);
    }
};

// ─── App globals ─────────────────────────────────────────────────────────────
static Camera   g_cam;
static SolarSim g_sim;
static double   g_lx=0, g_ly=0;
static bool     g_drag=false;
static int      g_follow=-1;

// ─── Saturn ring mesh (XZ plane, multi-band) ─────────────────────────────────
static std::vector<float> buildRing(const glm::vec3& cen,
                                    float inner, float outer,
                                    int seg=100, int bands=6) {
    std::vector<float> v;
    float pi2=6.28318f;
    for (int b=0;b<bands;b++) {
        float t = (float)b/(bands-1);
        float r = inner + t*(outer-inner);
        float a = 0.18f + 0.42f*std::sin(t*3.14159f);
        for (int i=0;i<seg;i++) {
            float a0=(float)i/seg*pi2, a1=(float)(i+1)/seg*pi2;
            v.insert(v.end(),{
                cen.x+r*std::cos(a0), cen.y, cen.z+r*std::sin(a0), a,
                cen.x+r*std::cos(a1), cen.y, cen.z+r*std::sin(a1), a
            });
        }
    }
    return v;
}

// ─── Star field ──────────────────────────────────────────────────────────────
static int buildStars(GLuint& vao, GLuint& vbo, int count=2500) {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> d(-1.f,1.f), b(0.25f,1.f);
    std::vector<float> v;
    for (int i=0;i<count;i++) {
        glm::vec3 p;
        do { p={d(rng),d(rng),d(rng)}; } while(glm::length(p)<0.01f);
        p=glm::normalize(p)*650.f;
        float br=b(rng);
        v.insert(v.end(),{p.x,p.y,p.z, br,br,br+0.05f, 0.004f, 1.f});
    }
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,v.size()*4,v.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,32,(void*)12);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,32,(void*)24);
    glEnableVertexAttribArray(3); glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,32,(void*)28);
    glBindVertexArray(0);
    return count;
}

// ─── Grid ────────────────────────────────────────────────────────────────────
static int buildGrid(GLuint& vao, GLuint& vbo) {
    std::vector<float> v;
    float sz=80.f; int n=32; float s=sz*2.f/n;
    for (int i=0;i<=n;i++) {
        float t=-sz+i*s;
        v.insert(v.end(),{t,0,-sz, t,0,sz, -sz,0,t, sz,0,t});
    }
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,v.size()*4,v.data(),GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,12,(void*)0);
    glBindVertexArray(0);
    return (int)v.size()/3;
}

// ─── Callbacks ───────────────────────────────────────────────────────────────
static void onKey(GLFWwindow* w, int key, int, int action, int) {
    if (action!=GLFW_PRESS) return;
    if (key==GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w,true);
    if (key==GLFW_KEY_SPACE)  g_sim.paused=!g_sim.paused;
    if (key==GLFW_KEY_T)      g_sim.showTrails=!g_sim.showTrails;
    if (key==GLFW_KEY_R)    { g_sim.init(); g_follow=-1; g_cam=Camera{}; }
    if (key==GLFW_KEY_F) {
        int total=(int)g_sim.bodies.size();
        for (int i=1;i<=total;i++) {
            int idx=(g_follow+i)%total;
            if (!g_sim.bodies[idx].isAsteroid) { g_follow=idx; return; }
        }
        g_follow=-1;
    }
}
static void onBtn(GLFWwindow*, int btn, int action, int) {
    if (btn==GLFW_MOUSE_BUTTON_LEFT) g_drag=(action==GLFW_PRESS);
}
static void onMove(GLFWwindow*, double x, double y) {
    if (g_drag) {
        g_cam.yaw  +=(float)(x-g_lx)*0.38f;
        g_cam.pitch-=(float)(y-g_ly)*0.38f;
        g_cam.pitch=glm::clamp(g_cam.pitch,-89.f,89.f);
    }
    g_lx=x; g_ly=y;
}
static void onScroll(GLFWwindow*, double, double dy) {
    g_cam.radius=glm::clamp(g_cam.radius-(float)dy*2.5f,1.f,600.f);
}

// ─── Main ────────────────────────────────────────────────────────────────────
int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES,4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // Borderless windowed fullscreen (OBS-friendly)
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);   // no title bar
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);      // stay on top
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* win = glfwCreateWindow(mode->width, mode->height,
        "Solar System",
        nullptr, nullptr);  // ← nullptr = NOT exclusive fullscreen

    // Position it at top-left so it covers the whole screen
    glfwSetWindowPos(win, 0, 0);

    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win); glfwSwapInterval(1);
    glfwSetKeyCallback(win,onKey);
    glfwSetMouseButtonCallback(win,onBtn);
    glfwSetCursorPosCallback(win,onMove);
    glfwSetScrollCallback(win,onScroll);

    glewExperimental=GL_TRUE;
    if (glewInit()!=GLEW_OK) return 1;

    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glEnable(GL_PROGRAM_POINT_SIZE); glEnable(GL_MULTISAMPLE);
    glClearColor(0.008f,0.008f,0.025f,1.f);

    GLuint planProg=linkProg(PLANET_VERT,PLANET_FRAG);
    GLuint trailProg=linkProg(TRAIL_VERT, TRAIL_FRAG);
    GLuint ringProg=linkProg(RING_VERT,  RING_FRAG);
    GLuint gridProg=linkProg(GRID_VERT,  GRID_FRAG);

    // Planet VAO: pos(3) col(3) radius(1) emissive(1) = 32 bytes
    GLuint pVAO,pVBO;
    glGenVertexArrays(1,&pVAO); glGenBuffers(1,&pVBO);
    glBindVertexArray(pVAO); glBindBuffer(GL_ARRAY_BUFFER,pVBO);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,32,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,32,(void*)12);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,32,(void*)24);
    glEnableVertexAttribArray(3); glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,32,(void*)28);
    glBindVertexArray(0);

    // Trail VAO: pos(3) col(3) age(1) = 28 bytes
    GLuint tVAO,tVBO;
    glGenVertexArrays(1,&tVAO); glGenBuffers(1,&tVBO);
    glBindVertexArray(tVAO); glBindBuffer(GL_ARRAY_BUFFER,tVBO);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,28,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,28,(void*)12);
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,28,(void*)24);
    glBindVertexArray(0);

    // Ring VAO: pos(3) alpha(1) = 16 bytes
    GLuint rVAO,rVBO;
    glGenVertexArrays(1,&rVAO); glGenBuffers(1,&rVBO);
    glBindVertexArray(rVAO); glBindBuffer(GL_ARRAY_BUFFER,rVBO);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,16,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,16,(void*)12);
    glBindVertexArray(0);

    GLuint sVAO,sVBO; int starCount=buildStars(sVAO,sVBO);
    GLuint gVAO,gVBO; int gridCount=buildGrid(gVAO,gVBO);

    g_sim.init();

    int frameN=0; float fps=60.f;
    auto last=std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        auto now=std::chrono::steady_clock::now();
        float dt=std::chrono::duration<float>(now-last).count();
        last=now;
        fps=fps*0.94f+(1.f/std::max(dt,0.001f))*0.06f;

        int substeps=std::min(8,std::max(1,(int)(dt/DT)));
        for (int s=0;s<substeps;s++) g_sim.step();

        // Follow target
        if (g_follow>=0 && g_follow<(int)g_sim.bodies.size())
            g_cam.target=g_sim.bodies[g_follow].pos;
        else
            g_cam.target=g_sim.bodies[0].pos;

        int w,h; glfwGetFramebufferSize(win,&w,&h);
        glViewport(0,0,w,h);
        float asp=(float)w/h;

        glm::mat4 V=g_cam.view(), P=g_cam.proj(asp), MVP=P*V;
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm::vec3 sunView=glm::vec3(V*glm::vec4(g_sim.bodies[0].pos,1.f));

        // Stars
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glUseProgram(planProg);
        glUniformMatrix4fv(glGetUniformLocation(planProg,"uMV"),1,GL_FALSE,glm::value_ptr(V));
        glUniformMatrix4fv(glGetUniformLocation(planProg,"uP"),1,GL_FALSE,glm::value_ptr(P));
        glUniform1f(glGetUniformLocation(planProg,"uViewH"),(float)h);
        glUniform3fv(glGetUniformLocation(planProg,"uLightView"),1,glm::value_ptr(sunView));
        glBindVertexArray(sVAO); glDrawArrays(GL_POINTS,0,starCount);
        glDepthMask(GL_TRUE);

        // Grid
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(gridProg);
        glUniformMatrix4fv(glGetUniformLocation(gridProg,"uMVP"),1,GL_FALSE,glm::value_ptr(MVP));
        glBindVertexArray(gVAO); glDrawArrays(GL_LINES,0,gridCount);

        // Trails
        if (g_sim.showTrails) {
            std::vector<float> td;
            for (auto& b:g_sim.bodies) {
                if (!b.showTrail||b.trailCount<2) continue;
                for (int k=0;k<b.trailCount-1;k++) {
                    int i0=(b.trailHead-1-k+TRAIL_LEN)%TRAIL_LEN;
                    int i1=(b.trailHead-2-k+TRAIL_LEN)%TRAIL_LEN;
                    float age=(float)k/b.trailCount;
                    td.insert(td.end(),{
                        b.trail[i0].x,b.trail[i0].y,b.trail[i0].z,
                        b.color.r,b.color.g,b.color.b,age,
                        b.trail[i1].x,b.trail[i1].y,b.trail[i1].z,
                        b.color.r,b.color.g,b.color.b,age+1.f/b.trailCount
                    });
                }
            }
            if (!td.empty()) {
                glBlendFunc(GL_SRC_ALPHA,GL_ONE);
                glBindVertexArray(tVAO); glBindBuffer(GL_ARRAY_BUFFER,tVBO);
                glBufferData(GL_ARRAY_BUFFER,td.size()*4,td.data(),GL_STREAM_DRAW);
                glUseProgram(trailProg);
                glUniformMatrix4fv(glGetUniformLocation(trailProg,"uMVP"),1,GL_FALSE,glm::value_ptr(MVP));
                glDrawArrays(GL_LINES,0,(GLsizei)(td.size()/7));
            }
        }

        // Rings
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(ringProg);
        glUniformMatrix4fv(glGetUniformLocation(ringProg,"uMVP"),1,GL_FALSE,glm::value_ptr(MVP));
        glBindVertexArray(rVAO); glBindBuffer(GL_ARRAY_BUFFER,rVBO);
        for (auto& b:g_sim.bodies) {
            if (!b.hasRing) continue;
            auto rv=buildRing(b.pos,b.ringInner,b.ringOuter);
            glBufferData(GL_ARRAY_BUFFER,rv.size()*4,rv.data(),GL_STREAM_DRAW);
            glDrawArrays(GL_LINES,0,(GLsizei)(rv.size()/4));
        }

        // Planets
        {
            std::vector<float> pd;
            for (auto& b:g_sim.bodies)
                pd.insert(pd.end(),{b.pos.x,b.pos.y,b.pos.z,
                    b.color.r,b.color.g,b.color.b,b.radius,b.isSun?1.f:0.f});
            glBlendFunc(GL_SRC_ALPHA,GL_ONE);
            glBindVertexArray(pVAO); glBindBuffer(GL_ARRAY_BUFFER,pVBO);
            glBufferData(GL_ARRAY_BUFFER,pd.size()*4,pd.data(),GL_STREAM_DRAW);
            glUseProgram(planProg);
            glUniformMatrix4fv(glGetUniformLocation(planProg,"uMV"),1,GL_FALSE,glm::value_ptr(V));
            glUniformMatrix4fv(glGetUniformLocation(planProg,"uP"),1,GL_FALSE,glm::value_ptr(P));
            glUniform1f(glGetUniformLocation(planProg,"uViewH"),(float)h);
            glUniform3fv(glGetUniformLocation(planProg,"uLightView"),1,glm::value_ptr(sunView));
            glDrawArrays(GL_POINTS,0,(GLsizei)g_sim.bodies.size());
        }

        // Title HUD
        if (++frameN%30==0) {
            std::string fl=(g_follow>=0)?" | Following: "+g_sim.bodies[g_follow].name:"";
            std::ostringstream ss;
            ss<<"Solar System | "<<(int)fps<<" fps | "
              <<(g_sim.paused?"PAUSED":"running")<<fl
              <<"  Space=Pause  R=Reset  T=Trails  F=Follow  Drag/Scroll=Camera";
            glfwSetWindowTitle(win,ss.str().c_str());
        }

        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1,&pVAO); glDeleteBuffers(1,&pVBO);
    glDeleteVertexArrays(1,&tVAO); glDeleteBuffers(1,&tVBO);
    glDeleteVertexArrays(1,&rVAO); glDeleteBuffers(1,&rVBO);
    glDeleteVertexArrays(1,&gVAO); glDeleteBuffers(1,&gVBO);
    glDeleteVertexArrays(1,&sVAO); glDeleteBuffers(1,&sVBO);
    glDeleteProgram(planProg); glDeleteProgram(trailProg);
    glDeleteProgram(ringProg); glDeleteProgram(gridProg);
    glfwDestroyWindow(win); glfwTerminate();
    return 0;
}
