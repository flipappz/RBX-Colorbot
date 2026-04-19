#include "Helpers.hpp"

/*config.txt to change settings, they'll auto load
see Helpers.hpp to learn how the colorbot works*/

inline bool Detect( CaptureCtxT* Ctx, ConfigT* Cfg ) {
    POINT Cursor;
    GetCursorPos( &Cursor );

    int Half = Ctx->Size >> 1;

    if ( !BitBlt( Ctx->MemDc, 0, 0,
             Ctx->Size, Ctx->Size,
             Ctx->ScreenDc,
             Cursor.x - Half, Cursor.y - Half,
             SRCCOPY ) )
        return false;

    BITMAPINFO Bmi{ };
    Bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    Bmi.bmiHeader.biWidth = Ctx->Size;
    Bmi.bmiHeader.biHeight = -Ctx->Size;
    Bmi.bmiHeader.biPlanes = 1;
    Bmi.bmiHeader.biBitCount = 32;

    if ( !GetDIBits( Ctx->MemDc, Ctx->Bmp, 0,
             Ctx->Size,
             Ctx->Buffer,
             &Bmi, DIB_RGB_COLORS ) )
        return false;

    int TolSq = Cfg->ColorTolerance * Cfg->ColorTolerance;

    uint8_t* Base = Ctx->Buffer;

    for ( int Y = 0; Y < Ctx->Size; Y += Cfg->ScanStride ) {
        uint8_t* Row = Base + ( Y * Ctx->Size * 4 );

        for ( int X = 0; X < Ctx->Size; X += Cfg->ScanStride ) {
            uint8_t* Px = Row + ( X * 4 );

            uint8_t B = Px[ 0 ];
            uint8_t G = Px[ 1 ];
            uint8_t R = Px[ 2 ];

            float Hue = RgbToHue( R, G, B );

            for ( TargetT* T = Targets;
                T < Targets + ( sizeof( Targets ) / sizeof( TargetT ) );
                ++T ) {

                if ( ColorDistSq( R, G, B, T->R, T->G, T->B ) <= TolSq ) {
                    float Diff = fabsf( Hue - T->HueCenter );
                    if ( Diff > 180.f )
                        Diff = 360.f - Diff;

                    if ( Diff <= T->HueTolerance )
                        return true;
                }
            }

            if ( ( B > 150 && B > R + 40 && B > G + 40 ) ||
                 ( R > 150 && R > G + 40 && R > B + 40 ) )
                return true;
        }
    }

    return false;
}

int main( ) {
    ConfigT Cfg{ };
    LoadConfig( "config.txt", &Cfg );

    printf( "hold ctrl to enable wallahi config muhamed" );

    CaptureCtxT Ctx{ };
    if ( !Ctx.Init( Cfg.DetectionSize ) )
        return 1;

    auto LastClick = std::chrono::high_resolution_clock::now( );

    while ( true ) {
        if ( IsKeyPressed( Cfg.ActivationKey ) ) {
            if ( Detect( &Ctx, &Cfg ) ) {
                auto Now = std::chrono::high_resolution_clock::now( );
                auto Ms = std::chrono::duration_cast< std::chrono::milliseconds >( Now - LastClick ).count( );

                if ( Ms >= Cfg.ClickCooldownMs ) {
                    INPUT In{ };
                    In.type = INPUT_MOUSE;

                    In.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput( 1, &In, sizeof( INPUT ) );

                    In.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput( 1, &In, sizeof( INPUT ) );

                    LastClick = Now;
                }
            }
        }

        Sleep( 1 );
    }

    Ctx.Destroy( );
    return 0;
}