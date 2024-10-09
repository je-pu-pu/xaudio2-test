#include <xaudio2.h>
#include <xapofx.h>
#include <iostream>
#include <cmath>
#include <vector>

// #pragma comment(lib, "XAPOFX.lib")

// �萔�ݒ�
const int SAMPLE_RATE = 44100;    // �T���v�����[�g�i44.1kHz�j
const float DURATION = 0.5f;      // �Đ����ԁi�b�j

IXAudio2* pXAudio2 = nullptr;

// �T�C���g�𐶐�����֐�
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

    // WAVEFORMATEX �\���̂̐ݒ�i�T�C���g�̃t�H�[�}�b�g�j
    WAVEFORMATEX waveFormat = { 0 };
    waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;  // ���������_���`��
    waveFormat.nChannels = 1;                        // ���m����
    waveFormat.nSamplesPerSec = SAMPLE_RATE;         // �T���v�����[�g
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * sizeof(float);  // ���σo�C�g��
    waveFormat.nBlockAlign = sizeof(float);          // �u���b�N�A���C��
    waveFormat.wBitsPerSample = 32;                  // 32�r�b�g���������_
    waveFormat.cbSize = 0;

    // �\�[�X�{�C�X�̍쐬
    HRESULT hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &waveFormat);
    if (FAILED(hr)) {
        throw "�\�[�X�{�C�X�̍쐬�Ɏ��s���܂����B";
    }

    // �T�C���g�f�[�^�̐���
    std::vector<BYTE>& waveData = *( new std::vector<BYTE>() );
    GenerateSineWave(waveData, SAMPLE_RATE, hz, DURATION);

    // �T�E���h�o�b�t�@�̐ݒ�
    XAUDIO2_BUFFER& buffer = *( new XAUDIO2_BUFFER{ 0 } );
    buffer.AudioBytes = static_cast<UINT32>(waveData.size());
    buffer.pAudioData = waveData.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    // �o�b�t�@���\�[�X�{�C�X�ɑ��M
    hr = pSourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        throw "�o�b�t�@�̑��M�Ɏ��s���܂����B";
    }

    return pSourceVoice;
}

int main() {
    // COM
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::cerr << "COM �̏������Ɏ��s���܂����B" << std::endl;
        return hr;
    }

    // XAudio2 �̏�����
    IXAudio2MasteringVoice* pMasterVoice = nullptr;

    hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        std::cerr << "XAudio2 �̏������Ɏ��s���܂����B" << std::endl;
        return -1;
    }

    // �}�X�^�[�{�C�X�̍쐬
    hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
    if (FAILED(hr)) {
        std::cerr << "�}�X�^�[�{�C�X�̍쐬�Ɏ��s���܂����B" << std::endl;
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

    // �Đ��J�n
    hr = a->Start( 0 );

    if ( FAILED(hr) )
    {
        std::cerr << "�Đ��Ɏ��s���܂����B" << std::endl;
        return -1;
    }
    b->Start( 0 );
    c->Start( 0 );

    // �Đ����̑ҋ@
    std::cout << "�T�C���g���Đ���..." << std::endl;
    Sleep(static_cast<DWORD>(10 * 1000));  // �Đ����Ԃ����ҋ@

    // �I������
    pXAPO->Release();

    a->DestroyVoice();
    b->DestroyVoice();
    c->DestroyVoice();

    pMasterVoice->DestroyVoice();
    pXAudio2->Release();

    ::CoUninitialize();

    return 0;
}
