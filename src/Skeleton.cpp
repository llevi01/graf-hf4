//=============================================================================================
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Lenart Levente
// Neptun : YIWROG
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

GPUProgram gpuProgram;

const char * const vertexSource = R"(
	#version 330
	precision highp float;

    layout(location = 0) in vec2 vp;
    layout(location = 1) in vec2 tp;

    out vec2 texPosition;

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1);
        texPosition = tp;
	}
)";

const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	
	uniform sampler2D textureSampler;

    in vec2 texPosition;
	out vec4 outColor;

	void main() {
		outColor = texture(textureSampler, texPosition);
	}
)";

vec3 operator/(vec3 lhs, vec3 rhs) {
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

class Material {
public:
    Material() {}
    virtual bool isRough() = 0;
    virtual bool isReflective() = 0;
    virtual bool isRefractive() = 0;
};

class RoughMaterial {
private:
    vec3 ka, kd, ks;
    float shine;

public:
    RoughMaterial(vec3 ka, vec3 kd, vec3 ks, float shine) {
        this->ka = ka;
        this->kd = kd;
        this->ks = ks;
        this->shine = shine;
    }

    bool isRough() { return true; }
    bool isReflective() { return false; }
    bool isRefractive() { return false; }
};

class SmoothMaterial {
    vec3 n, kappa, f0;
    bool refractive;

private:
    SmoothMaterial(vec3 n, vec3 kappa, bool refractive) {
        this->n = n;
        this->kappa = kappa;
        this->refractive = refractive;

        vec3 unitVec = (1, 1, 1);
        this->f0 = ((n - unitVec) * (n - unitVec) + kappa * kappa) /
                   ((n + unitVec) * (n + unitVec) + kappa * kappa);
    }

    bool isRough() { return false; }
    bool isReflective() { return true; }
    bool isRefractive() { return this->refractive; }

    vec3 reflect(vec3 direction, vec3 normalVector) {
        return direction - normalVector * dot(direction, normalVector) * 2;
    }

    vec3 fresnel(vec3 direction, vec3 normalVector) {
        vec3 unitVector = (1, 1, 1);
        return f0 + (unitVector - f0) * powf(1 + dot(direction, normalVector), 5); // TODO ez lehet nem jó
    }
};

struct Hit {
    float param;
    vec3 coord, normalVector;
    Material* material;

};

struct Ray {
    vec3 startPoint, direction;

    Ray(vec3 startPoint, vec3 direction) {
        this->startPoint = startPoint;
        this->direction = direction;
    }
};

class Intersectable {
protected:
    Material* material;

public:
    virtual Hit getIntersection(Ray& ray) = 0;
};

class Plane : public Intersectable {
    int height, width, depth;
    Material *material1, *material2;

public:
    Plane(int height, int width, int depth, Material *material1, Material *material2) {
        this->height = height;
        this->width = width;
        this->depth = depth;
        this->material1 = material1;
        this->material2 = material2;
    }
};

class Cylinder : public Intersectable {

};

class Cone : public Intersectable {

};

class Camera {
    vec3 eye, lookat, right, up;
public:


    void set(vec3 _eye, vec3 _lookat, vec3 vup, float fov) {
        eye = _eye;
        lookat = _lookat;
        vec3 w = eye - lookat;
        float focus = length(w);
        right = normalize(cross(vup, w)) * focus * tanf(fov / 2);
        up = normalize(cross(w, right)) * focus * tanf(fov / 2);
    }

    Ray getRay(int X, int Y) {
        vec3 dir = lookat + right * (2.0f * (X + 0.5f) / windowWidth - 1) + up * (2.0f * (Y + 0.5f) / windowHeight - 1) - eye;
        return {eye, dir};
    }
};

class World {

};

// ------- KI KELL TÖRÖLNI -------
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
             ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
             type, severity, message );
}

unsigned vao, vbos[2];

unsigned int textureId;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

    // ------- KI KELL TÖRÖLNI -------
    glEnable              ( GL_DEBUG_OUTPUT );
    glDebugMessageCallback( MessageCallback, nullptr );

    std::vector<vec2> rectangleCords, textureCoords;

    rectangleCords.emplace_back(-1, -1);
    rectangleCords.emplace_back(-1, 1);
    rectangleCords.emplace_back(1, 1);
    rectangleCords.emplace_back(1, -1);

    textureCoords.emplace_back(0, 0);
    textureCoords.emplace_back(0, 1);
    textureCoords.emplace_back(1, 1);
    textureCoords.emplace_back(1, 0);

    glGenVertexArrays(1, &vao);
    glGenBuffers(2, vbos);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 4, &rectangleCords[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 4, &textureCoords[0], GL_STATIC_DRAW);

	glGenTextures(1, &textureId);

	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

    // TODO std::vector<vec4> texture = scene.draw();
    std::vector<vec4> texture;
    for (int i = 0; i < windowWidth * windowHeight; ++i) {
        texture.emplace_back(1, 0, 0, 1);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, &texture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int samplerLocation = glGetUniformLocation(gpuProgram.getId(), "textureSampler");
    glUniform1i(samplerLocation, 0);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glutSwapBuffers();
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();
}

void onKeyboardUp(unsigned char key, int pX, int pY) {}

void onMouseMotion(int pX, int pY) {}

void onMouse(int button, int state, int pX, int pY) {}

void onIdle() {}
