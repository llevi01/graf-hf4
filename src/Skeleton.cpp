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

float epsilon = 0.001f;

vec3 operator/(vec3 lhs, vec3 rhs) {
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

vec3 sqrt(vec3 vec) {
    return {sqrtf(vec.x), sqrtf(vec.y), sqrtf(vec.z)};
}

struct Material {
public:
    virtual bool isRough() = 0;
    virtual bool isReflective() = 0;
    virtual bool isRefractive() = 0;
};

struct RoughMaterial : public Material {
public:
    vec3 ka, kd, ks;
    float shine;

    RoughMaterial(vec3 kd, vec3 ks, float shine) {
        this->ka = kd * 3;
        this->kd = kd;
        this->ks = ks;
        this->shine = shine;
    }

    bool isRough() override { return true; }
    bool isReflective() override { return false; }
    bool isRefractive() override { return false; }
};

struct SmoothMaterial : public Material {
public:
    vec3 n, kappa, f0;
    bool refractive;

    SmoothMaterial(vec3 n, vec3 kappa, bool refractive = false) {
        this->n = n;
        this->kappa = kappa;
        this->refractive = refractive;

        vec3 unitVec(1, 1, 1);
        this->f0 = ((n - unitVec) * (n - unitVec) + kappa * kappa) /
                   ((n + unitVec) * (n + unitVec) + kappa * kappa);
    }

    bool isRough() override { return false; }
    bool isReflective() override { return true; }
    bool isRefractive() override { return this->refractive; }

    vec3 reflect(vec3 direction, vec3 normalVector) {
        return normalize(direction - normalVector * dot(direction, normalVector) * 2.0f);
    }

    vec3 refract(vec3 direction, vec3 normalVector, bool isInside) {
        vec3 fractionIndex = isInside ? vec3(1, 1, 1) / n : n;
        float a = M_PI - acosf(dot(direction, normalVector) / (length(direction) * length(normalVector)));
        float cosa = cosf(a);
        float d = 1 - (1 - powf(cosa, 2)) / dot(fractionIndex, fractionIndex);
        if (d < epsilon) {
            return {0, 0, 0};
        }
        float sqrtd = sqrtf(d);
        return normalize(direction / fractionIndex + direction * (vec3(cosa, cosa, cosa) / fractionIndex - vec3(sqrtd, sqrtd, sqrtd)));
    }

    vec3 fresnel(vec3 direction, vec3 normalVector) {
        vec3 unitVector(1, 1, 1);
        float cosa = -dot(direction, normalVector);
        return f0 + (unitVector - f0) * powf(1 - cosa, 5);
    }
};

struct Hit {
public:
    float param;
    vec3 coord, normalVec;
    Material* material;

    Hit() {
        param = -1;
    }

    Hit(float param, vec3 coord, vec3 normalVec, Material *material) {
        this->param = param;
        this->coord = coord;
        this->normalVec = normalVec;
        this->material = material;
    }
};

struct Ray {
public:
    vec3 startPoint, dir;
    bool isInside;

    Ray(vec3 startPoint, vec3 direction, bool isInside = false) {
        this->startPoint = startPoint;
        this->dir = normalize(direction);
        this->isInside = isInside;
    }
};

class Camera {
    vec3 eyePos, lookatPos, rightVec, upVec, preferredUpDir;
    float fov;

    void doCalc() {
        vec3 lookatVec = eyePos - lookatPos;
        float focusLen = length(lookatVec);
        this->rightVec = normalize(cross(preferredUpDir, lookatVec)) * focusLen * tanf(fov / 2.0f);
        this->upVec = normalize(cross(lookatVec, rightVec)) * focusLen * tanf(fov / 2.0f);
    }

public:
    Camera(vec3 eyePos, vec3 lookatPos, vec3 preferredUpDir, float fov) {
        this->eyePos = eyePos;
        this->lookatPos = lookatPos;
        this->fov = fov;
        this->preferredUpDir = normalize(preferredUpDir);
        doCalc();
    }

    Ray getRay(int X, int Y) {
        vec3 dir = lookatPos + rightVec * (2.0f * (X + 0.5f) / (float) windowWidth - 1) + upVec * (2.0f * (Y + 0.5f) / (float) windowHeight - 1) - eyePos;
        return Ray(eyePos, normalize(dir));
    }

    void rotate() {
        vec4 temp = vec4(eyePos.x, eyePos.y, eyePos.z, 1) * RotationMatrix(M_PI / 4.0f, vec3(0, 1, 0));
        eyePos = vec3(temp.x, temp.y, temp.z);
        doCalc();
    }
};

struct DirectionalLight {
    vec3 dir, intensity;
    DirectionalLight(vec3 direction, vec3 intensity) {
        this->dir = normalize(direction);
        this->intensity = intensity;
    }
};

class Intersectable {
protected:
    Material *material;
public:
    virtual Hit getIntersection(Ray& ray) = 0;
};

class Plane : public Intersectable {
    int height, width, depth;
    Material *material2;

public:
    Plane(int height, int width, int depth, Material *material1, Material *material2) {
        this->height = height;
        this->width = width;
        this->depth = depth;
        this->material = material1;
        this->material2 = material2;
    }

    Hit getIntersection(Ray& ray) override {
        vec3 normalVec(0, 1, 0);
        vec3 pointOnPlane(0, height, 0);
        float denom = dot(ray.dir, normalVec);
        if (abs(denom) < epsilon) {
            return Hit();
        }
        float nom = dot(pointOnPlane - ray.startPoint, normalVec);
        float t = nom / denom;
        if (t < epsilon) {
            return Hit();
        }
        vec3 intersection = ray.startPoint + t * ray.dir;
        vec3 refPoint = vec3(-width / 2.0f, height, -depth / 2.0f);
        vec3 distanceVec = intersection - refPoint;
        if (distanceVec.x < 0 || distanceVec.z < 0 ||
            distanceVec.x > width || distanceVec.z > depth) {
            return Hit();
        }
        int a = (int) distanceVec.x % 2;
        int b = (int) distanceVec.z % 2;

        if (a + b == 1) {
            return Hit(t, intersection, normalVec, material);
        } else {
            return Hit(t, intersection, normalVec, material2);
        }
    }
};

class Cone : public Intersectable {
private:
    vec3 startPoint, dir;
    float angle, height;

public:
    Cone(vec3 startPoint, vec3 dir, float angle, float height, Material *material) {
        this->startPoint = startPoint;
        this->dir = normalize(dir);
        this->angle = angle;
        this->height = height;
        this->material = material;
    }

    Hit getIntersection(Ray& ray) override {
        // Forras: https://hugi.scene.org/online/hugi24/coding%20graphics%20chris%20dragan%20raytracing%20shapes.htm
        vec3 temp1 = ray.startPoint - startPoint;
        float tana = tanf(angle);
        float temp2 = (1 + tana * tana);

        float a = dot(ray.dir, ray.dir) - temp2 * powf(dot(ray.dir, dir), 2);
        float b = 2 * (dot(ray.dir, temp1) - temp2 * dot(ray.dir, dir) * dot(temp1, dir));
        float c = dot(temp1, temp1) - temp2 * powf(dot(temp1, dir), 2);
        float d = b * b - 4 * a * c;

        if (d < 0) {
            return Hit();
        }

        float t;
        float t1 = (- b + sqrtf(d)) / 2.0f / a;
        float t2 = (- b - sqrtf(d)) / 2.0f / a;
        if (t1 <= 0) {
            return Hit();
        }
        t = t2 > 0 ? t2 : t1;

        vec3 hitPoint = ray.startPoint + t * ray.dir;
        float hitDist = dot(dir, ray.dir) * t + dot(temp1, dir);
        if (hitDist < 0 || hitDist > height) {
            return Hit();
        }
        float r = tana * height;
        float temp3 = r / hitDist;
        vec3 normalVec = normalize(hitPoint - startPoint - dir * hitDist - dir * hitDist * temp3 * temp3);

        return {t, hitPoint, normalVec, material};
    }
};

class Cylinder : public Intersectable {
private:
    vec3 startPoint, dir;
    float radius, height;

public:
    Cylinder(vec3 startPoint, vec3 dir, float radius, float height, Material *material) {
        this->startPoint = startPoint;
        this->dir = normalize(dir);
        this->radius = radius;
        this->height = height;
        this->material = material;
    }

    Hit getIntersection(Ray& ray) override {
        // Forras: https://hugi.scene.org/online/hugi24/coding%20graphics%20chris%20dragan%20raytracing%20shapes.htm
        vec3 temp = ray.startPoint - startPoint;

        float a = dot(ray.dir, ray.dir) - powf(dot(ray.dir, dir), 2);
        float b = 2 * (dot(ray.dir, temp) - dot(ray.dir, dir) * dot(temp, dir));
        float c = dot(temp, temp) - powf(dot(temp, dir), 2) - radius * radius;
        float d = b * b - 4 * a * c;

        if (d < 0) {
            return Hit();
        }

        float t;
        float t1 = (-b + sqrtf(d)) / 2.0f / a;
        float t2 = (-b - sqrtf(d)) / 2.0f / a;
        if (t1 <= 0) {
            return Hit();
        }
        t = t2 > 0 ? t2 : t1;

        vec3 hitPoint = ray.startPoint + t * ray.dir;
        float hitDist = dot(dir, ray.dir) * t + dot(temp, dir);
        if (hitDist < 0 || hitDist > height) {
            return Hit();
        }

        vec3 normalVec = normalize(hitPoint - startPoint - dir * hitDist);
        return Hit(t, hitPoint, normalVec, material);
    }
};

class World {
private:
    std::vector<Intersectable*> shapes;
    std::vector<Material*> materials;
    DirectionalLight directionalLight;
    vec3 ambientLight;
    Camera camera;

public:
    World() : directionalLight(vec3(1, 1, 1), vec3(2, 2, 2)),
              ambientLight(0.4, 0.4, 0.4),
              camera(vec3(0, 1, 4), vec3(0, 0, 0), vec3(0, 1, 0), (45.0f / 180.0f * M_PI))
    {
        Material *blue = new RoughMaterial(vec3(0, 0.1, 0.3), vec3(0, 0, 0), 0);
        materials.push_back(blue);

        Material *white = new RoughMaterial(vec3(0.3, 0.3, 0.3), vec3(0, 0, 0), 0);
        materials.push_back(white);

        Material *gold = new SmoothMaterial(vec3(0.17, 0.35, 1.5), vec3(3.1, 2.7, 1.9));
        materials.push_back(gold);

        Material *water = new SmoothMaterial(vec3(1.3, 1.3, 1.3), vec3(0, 0, 0), true);
        materials.push_back(water);

        Material *orangePlastic = new RoughMaterial(vec3(0.3, 0.2, 0.1), vec3(2, 2, 2), 50);
        materials.push_back(orangePlastic);

        Material *cyan = new RoughMaterial(vec3(0.1, 0.2, 0.3), vec3(2, 2, 2), 100);
        materials.push_back(cyan);

        Material *magenta = new RoughMaterial(vec3(0.3, 0, 0.2), vec3(2, 2, 2), 20);
        materials.push_back(magenta);

        shapes.push_back(new Plane(-1, 20, 20, blue, white));
        shapes.push_back(new Cylinder(vec3(1, -1, 0), vec3(0.1, 1, 0), 0.3, 2, gold));
        shapes.push_back(new Cylinder(vec3(0, -1, -0.8), vec3(-0.2, 1, -0.1), 0.3, 2, water));
        shapes.push_back(new Cylinder(vec3(-1, -1, 0), vec3(0, 1, 0.1), 0.3, 2, orangePlastic));
        shapes.push_back(new Cone(vec3(0, 1, 0), vec3(-0.1, -1, -0.05), 0.2, 2, cyan));
        shapes.push_back(new Cone(vec3(0, 1, 0.8), vec3(0.2, -1, 0), 0.2, 2, magenta));
    }

    Hit findIntersection(Ray& ray) {
        Hit res;

        for (Intersectable *shape : shapes) {
            Hit hit = shape->getIntersection(ray);
            if (hit.param > 0 && (res.param <= 0 || hit.param < res.param)) {
                res = hit;
            }
        }

        if (dot(ray.dir, res.normalVec) > 0) {
            res.normalVec = res.normalVec * -1.0f;
        }

        return res;
    }

    bool shadowIntersect(Ray& ray) {
        for (Intersectable* shape : shapes) {
           if (shape->getIntersection(ray).param > 0){
               return true;
           }
        }
        return false;
    }

    vec3 trace(Ray ray, int n = 0) {
        if (n > 5) {
            return ambientLight;
        }
        Hit hit = findIntersection(ray);
        if (hit.param <= 0) {
            return ambientLight;
        }

        vec3 res(0, 0, 0);
        if (hit.material->isRough()) {
            RoughMaterial* hitMateiral = dynamic_cast<RoughMaterial*> (hit.material);
            res = hitMateiral->ka * ambientLight;

            Ray shadowRay(hit.coord + hit.normalVec * epsilon, directionalLight.dir);
            float cosTheta = dot(hit.normalVec, directionalLight.dir);
            if (cosTheta > 0 && !shadowIntersect(shadowRay)) {
                res = res + directionalLight.intensity * hitMateiral->kd * cosTheta;
                vec3 halfway = normalize(-ray.dir + directionalLight.dir);
                float cosDelta = dot(hit.normalVec, halfway);
                if (cosDelta > 0) {
                    res = res + directionalLight.intensity * hitMateiral->ks * powf(cosDelta, hitMateiral->shine);
                }
            }
        }
        if (hit.material->isReflective()) {
            SmoothMaterial* hitMaterial = dynamic_cast<SmoothMaterial*> (hit.material);
            vec3 reflectionDir = hitMaterial->reflect(ray.dir, hit.normalVec);
            vec3 fresnel = hitMaterial->fresnel(ray.dir, hit.normalVec);
            res = res + trace(Ray(hit.coord + hit.normalVec * epsilon, reflectionDir), n + 1) * fresnel;
        }
        if (hit.material->isRefractive()) {
            SmoothMaterial* hitMaterial = dynamic_cast<SmoothMaterial*> (hit.material);
            vec3 refractionDir = hitMaterial->refract(ray.dir, hit.normalVec, ray.isInside);
            if (length(refractionDir) > 0) {
                res = res + trace(Ray(hit.coord - hit.normalVec * epsilon, refractionDir, !ray.isInside), n + 1) *
                    (vec3(1, 1, 1) - hitMaterial->fresnel(ray.dir, hit.normalVec));
            }
        }

        return res;
    }

    void render(std::vector<vec4>& texture) {
        for (int y = 0; y < windowHeight; ++y) {
            for (int x = 0; x < windowWidth; ++x) {
                vec3 color = this->trace(camera.getRay(x, y));
                texture.emplace_back(color.x, color.y, color.z, 1);
            }
        }
    }

    void rotateCamera() {
        camera.rotate();
    }

    ~World() {
        for(Material *material : materials) {
            delete material;
        }
        for (Intersectable *shape : shapes) {
            delete shape;
        }
    }
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
World world;

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

    std::vector<vec4> texture;
    world.render(texture);

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
	if (key == 'a') {
	    world.rotateCamera();
	    glutPostRedisplay();
	}
}

void onKeyboardUp(unsigned char key, int pX, int pY) {}

void onMouseMotion(int pX, int pY) {}

void onMouse(int button, int state, int pX, int pY) {}

void onIdle() {}
