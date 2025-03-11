#include <windows.h>
#include <math.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <vector>
#include <bits/stdc++.h>

class Window
{
private:
    struct vec3d
    {
        float x, y, z;
    };

    struct triangle
    {
        vec3d p[3];
    };

    struct mesh
    {
        std::vector<triangle> tris;
    };

    struct mat4x4
    {
        float m[4][4] = {0};
    };

    HANDLE consoleIn;
    HANDLE consoleOut;
    CONSOLE_SCREEN_BUFFER_INFO screen;
    INPUT_RECORD inputRecord;
    DWORD numRead;
    short columns;
    short rows;
    char *clearBuffer;
    char *buffer;
    mesh meshCube;
    mat4x4 matProj;
    vec3d vCamera;
    float theta;
    long long int deltaTime;
    int newFd;
    int pow10[3] = {1, 10, 100};

    void plotPoint(int x, int y, int color)
    {
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                this->buffer[(y * this->columns + x) * 20 + 4 * i + 13 - j] = 48 + ((color >> 8 * i) & 0xFF) / pow10[j] % 10;
            }
        }
    }
    
    void plotCharacter(int x, int y, char character) {
        this->buffer[(y * this->columns + x) * 20 + 23] = character;
    }

    void plotLine(int x1, int y1, int x2, int y2, int color) // EFLA E go zoom
    {
        bool yLonger = false;
        int shortLen = y2 - y1;
        int longLen = x2 - x1;
        if (abs(shortLen) > abs(longLen))
        {
            int swap = shortLen;
            shortLen = longLen;
            longLen = swap;
            yLonger = true;
        }
        int decInc;
        if (longLen == 0)
            decInc = 0;
        else
            decInc = (shortLen << 16) / longLen;

        if (yLonger)
        {
            if (longLen > 0)
            {
                longLen += y1;
                for (int j = 0x8000 + (x1 << 16); y1 <= longLen; ++y1)
                {
                    plotPoint(j >> 16, y1, color);
                    j += decInc;
                }
                return;
            }
            longLen += y1;
            for (int j = 0x8000 + (x1 << 16); y1 >= longLen; --y1)
            {
                plotPoint(j >> 16, y1, color);
                j -= decInc;
            }
            return;
        }

        if (longLen > 0)
        {
            longLen += x1;
            for (int j = 0x8000 + (y1 << 16); x1 <= longLen; ++x1)
            {
                plotPoint(x1, j >> 16, color);
                j += decInc;
            }
            return;
        }
        longLen += x1;
        for (int j = 0x8000 + (y1 << 16); x1 >= longLen; --x1)
        {
            plotPoint(x1, j >> 16, color);
            j -= decInc;
        }
    }

    void plotTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color)
    {
        plotLine(x1, y1, x2, y2, color);
        plotLine(x2, y2, x3, y3, color);
        plotLine(x3, y3, x1, y1, color);
    }

    // void plotString(int x, int y, std::string s)
    // {
    //     for (int i = 0; i < s.size(); i++)
    //     {
    //     }
    // }

    void multiplyMatrixVector(vec3d &i, vec3d &o, mat4x4 &m)
    {
        o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
        o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
        o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];
        float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];

        if (w != 0.0f)
        {
            o.x /= w;
            o.y /= w;
            o.z /= w;
        }
    }

    void clear()
    {
        strncpy(this->buffer, this->clearBuffer, this->rows * this->columns * 20 + 5);
    }

public:
    Window()
    {
        this->consoleIn = GetStdHandle(STD_INPUT_HANDLE);
        DWORD dwMode;
        GetConsoleMode(this->consoleIn, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(this->consoleIn, dwMode);
        this->consoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(this->consoleOut, &this->screen);
        this->columns = screen.dwMaximumWindowSize.X;
        this->rows = screen.dwMaximumWindowSize.Y;
        this->clearBuffer = new char[this->rows * this->columns * 20 + 4];
        this->buffer = new char[this->rows * this->columns * 20 + 4];

        strncpy(this->buffer, "\x1b[;H", 5);
        for (int i = 0; i < this->columns * this->rows; i++)
        {
            strncpy(this->buffer + 4 + i * 20, "\x1b[48;2;000;000;000m ", 21);
        }
        strncpy(this->clearBuffer, this->buffer, this->rows * this->columns * 20 + 5);

        this->meshCube = mesh();
        this->meshCube.tris = {

            // SOUTH
            {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f},

            // EAST
            {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},

            // NORTH
            {1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f},
            {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f},

            // WEST
            {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},

            // TOP
            {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},

            // BOTTOM
            {1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},

        };

        this->newFd = dup(STDOUT_FILENO);

        _write(this->newFd, "\x1b]0;Renderer\x07\x1b[?25l", 25);

        float fNear = 0.1f;
        float fFar = 1000.0f;
        float fFov = 90.0f;
        float fAspectRatio = (float)rows / (float)columns * 2;
        float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

        matProj.m[0][0] = fAspectRatio * fFovRad;
        matProj.m[1][1] = fFovRad;
        matProj.m[2][2] = fFar / (fFar - fNear);
        matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
        matProj.m[2][3] = 1.0f;
        matProj.m[3][3] = 0.0f;

        this->theta = 0;
        this->deltaTime = 0;
    }

    constexpr int getRGB(unsigned char r, unsigned char g, unsigned char b) {
        return ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF);
    }

    void update()
    {
        this->clear();
        this->theta += this->deltaTime * 0.000000001;

        mat4x4 matRotZ, matRotX = mat4x4();
        matRotZ.m[0][0] = cosf(this->theta);
        matRotZ.m[0][1] = sinf(this->theta);
        matRotZ.m[1][0] = -sinf(this->theta);
        matRotZ.m[1][1] = cosf(this->theta);
        matRotZ.m[2][2] = 1;
        matRotZ.m[3][3] = 1;

        matRotX.m[0][0] = 1;
        matRotX.m[1][1] = cosf(this->theta * 0.5f);
        matRotX.m[1][2] = sinf(this->theta * 0.5f);
        matRotX.m[2][1] = -sinf(this->theta * 0.5f);
        matRotX.m[2][2] = cosf(this->theta * 0.5f);
        matRotX.m[3][3] = 1;

        for (auto tri : meshCube.tris)
        {
            triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;

            // Rotate in Z-Axis
            multiplyMatrixVector(tri.p[0], triRotatedZ.p[0], matRotZ);
            multiplyMatrixVector(tri.p[1], triRotatedZ.p[1], matRotZ);
            multiplyMatrixVector(tri.p[2], triRotatedZ.p[2], matRotZ);

            // Rotate in X-Axis
            multiplyMatrixVector(triRotatedZ.p[0], triRotatedZX.p[0], matRotX);
            multiplyMatrixVector(triRotatedZ.p[1], triRotatedZX.p[1], matRotX);
            multiplyMatrixVector(triRotatedZ.p[2], triRotatedZX.p[2], matRotX);

            // Offset into the screen
            triTranslated = triRotatedZX;
            triTranslated.p[0].z = triRotatedZX.p[0].z + 3.0f;
            triTranslated.p[1].z = triRotatedZX.p[1].z + 3.0f;
            triTranslated.p[2].z = triRotatedZX.p[2].z + 3.0f;

            // Project triangles from 3D --> 2D
            multiplyMatrixVector(triTranslated.p[0], triProjected.p[0], matProj);
            multiplyMatrixVector(triTranslated.p[1], triProjected.p[1], matProj);
            multiplyMatrixVector(triTranslated.p[2], triProjected.p[2], matProj);

            // Scale into view
            triProjected.p[0].x += 1.0f;
            triProjected.p[0].y += 1.0f;
            triProjected.p[1].x += 1.0f;
            triProjected.p[1].y += 1.0f;
            triProjected.p[2].x += 1.0f;
            triProjected.p[2].y += 1.0f;
            triProjected.p[0].x *= 0.5f * (float)this->columns;
            triProjected.p[0].y *= 0.5f * (float)this->rows;
            triProjected.p[1].x *= 0.5f * (float)this->columns;
            triProjected.p[1].y *= 0.5f * (float)this->rows;
            triProjected.p[2].x *= 0.5f * (float)this->columns;
            triProjected.p[2].y *= 0.5f * (float)this->rows;
            this->plotTriangle(triProjected.p[0].x, triProjected.p[0].y,
                               triProjected.p[1].x, triProjected.p[1].y,
                               triProjected.p[2].x, triProjected.p[2].y,
                               this->getRGB(255, 0, 255));
        }
    }

    void display()
    {
        _write(this->newFd, this->buffer, this->rows * this->columns * 20 + 4);
    }

    void setDeltaTime(long long int deltaTime)
    {
        this->deltaTime = deltaTime;
    }
};

int main()
{
    std::ios_base::sync_with_stdio(false);
    Window window = Window();
    while (true)
    {
        auto start = std::chrono::high_resolution_clock::now();
        window.display();
        window.update();
        window.setDeltaTime(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count());
    }
}