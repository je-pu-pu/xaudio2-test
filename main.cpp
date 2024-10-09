#include <xaudio2.h>
#include <xapofx.h>
#include <iostream>
#include <cmath>
#include <vector>

// #pragma comment(lib, "XAPOFX.lib")

// 定数設定
const int SAMPLE_RATE = 44100;    // サンプルレート（44.1kHz）
const float DURATION = 0.5f;      // 再生時間（秒）

IXAudio2* pXAudio2 = nullptr;

// サイン波を生成する関数
void GenerateSineWave(std::vector<BYTE>& buffer, int sampleRate, int frequency, float duration) {
    int sampleCount = static_cast<int>(sampleRate * duration);
    buffer.resize(sampleCount * sizeof(float));

    float atack = 0.01f;
    float release_time = 0.1f;

    for (int i = 0; i < sampleCount; ++i) {
        // float level = sinf( 3.14159f * i / sampleRate * DURATION );
        float level = min( 1.f, i / ( sampleRate * atack ) );

        if ( i >= sampleRate * release_time )
        {
            level *= min( 1.f, ( ( 1.f - ( i / ( sampleCount - 1.f ) ) ) * ( DURATION / ( DURATION - release_time ) ) ) );
        }

        float sample = sinf(2 * 3.14159f * frequency * i / sampleRate) * level;
        reinterpret_cast<float*>(buffer.data())[i] = sample;
    }
}

IXAudio2SourceVoice* createSinVoice( int hz )
{
    IXAudio2SourceVoice* pSourceVoice = nullptr;

    // WAVEFORMATEX 構造体の設定（サイン波のフォーマット）
    WAVEFORMATEX waveFormat = { 0 };
    waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;  // 浮動小数点数形式
    waveFormat.nChannels = 1;                        // モノラル
    waveFormat.nSamplesPerSec = SAMPLE_RATE;         // サンプルレート
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * sizeof(float);  // 平均バイト数
    waveFormat.nBlockAlign = sizeof(float);          // ブロックアライン
    waveFormat.wBitsPerSample = 32;                  // 32ビット浮動小数点
    waveFormat.cbSize = 0;

    // ソースボイスの作成
    HRESULT hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &waveFormat);
    if (FAILED(hr)) {
        throw "ソースボイスの作成に失敗しました。";
    }

    // サイン波データの生成
    std::vector<BYTE>& waveData = *( new std::vector<BYTE>() );
    GenerateSineWave(waveData, SAMPLE_RATE, hz, DURATION);

    // サウンドバッファの設定
    XAUDIO2_BUFFER& buffer = *( new XAUDIO2_BUFFER{ 0 } );
    buffer.AudioBytes = static_cast<UINT32>(waveData.size());
    buffer.pAudioData = waveData.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    // バッファをソースボイスに送信
    hr = pSourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        throw "バッファの送信に失敗しました。";
    }

    return pSourceVoice;
}

int main() {
    // COM
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::cerr << "COM の初期化に失敗しました。" << std::endl;
        return hr;
    }

    // XAudio2 の初期化
    IXAudio2MasteringVoice* pMasterVoice = nullptr;

    hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        std::cerr << "XAudio2 の初期化に失敗しました。" << std::endl;
        return -1;
    }

    // マスターボイスの作成
    hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
    if (FAILED(hr)) {
        std::cerr << "マスターボイスの作成に失敗しました。" << std::endl;
        return -1;
    }

    auto a = createSinVoice( 264 );
    auto b = createSinVoice( 330 );
    auto c = createSinVoice( 396 );

    IUnknown * pXAPO;
    CreateFX( __uuidof( FXEcho ), & pXAPO );

    XAUDIO2_EFFECT_DESCRIPTOR descriptor;
    descriptor.InitialState = true;
    descriptor.OutputChannels = 1;
    descriptor.pEffect = pXAPO;

    XAUDIO2_EFFECT_CHAIN chain;
    chain.EffectCount = 1;
    chain.pEffectDescriptors = &descriptor;

    a->SetEffectChain( & chain );
    // b->SetEffectChain( & chain );
    // c->SetEffectChain( & chain );

    // 再生開始
    hr = a->Start( 0 );

    if ( FAILED(hr) )
    {
        std::cerr << "再生に失敗しました。" << std::endl;
        return -1;
    }
    b->Start( 0 );
    c->Start( 0 );

    // 再生中の待機
    std::cout << "サイン波を再生中..." << std::endl;
    Sleep(static_cast<DWORD>(10 * 1000));  // 再生時間だけ待機

    // 終了処理
    pXAPO->Release();

    a->DestroyVoice();
    b->DestroyVoice();
    c->DestroyVoice();

    pMasterVoice->DestroyVoice();
    pXAudio2->Release();

    ::CoUninitialize();

    return 0;
}
