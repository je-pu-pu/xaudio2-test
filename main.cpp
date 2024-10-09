#include <xaudio2.h>
#include <xapofx.h>
#include <iostream>
#include <cmath>
#include <vector>

// #pragma comment(lib, "XAPOFX.lib")

// �萔�ݒ�
const int SAMPLE_RATE = 44100;    // �T���v�����[�g�i44.1kHz�j
const int TONE_HZ = 440;          // �T�C���g�̎��g���i440Hz�AA���j
const float DURATION = 0.1f;      // �Đ����ԁi�b�j

// �T�C���g�𐶐�����֐�
void GenerateSineWave(std::vector<BYTE>& buffer, int sampleRate, int frequency, float duration) {
    int sampleCount = static_cast<int>(sampleRate * duration);
    buffer.resize(sampleCount * sizeof(float));

    for (int i = 0; i < sampleCount; ++i) {
        float sample = sinf(2 * 3.14159f * frequency * i / sampleRate);
        reinterpret_cast<float*>(buffer.data())[i] = sample;
    }
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
    IXAudio2* pXAudio2 = nullptr;
    IXAudio2MasteringVoice* pMasterVoice = nullptr;
    IXAudio2SourceVoice* pSourceVoice = nullptr;

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
    hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &waveFormat);
    if (FAILED(hr)) {
        std::cerr << "�\�[�X�{�C�X�̍쐬�Ɏ��s���܂����B" << std::endl;
        return -1;
    }

    // �T�C���g�f�[�^�̐���
    std::vector<BYTE> waveData;
    GenerateSineWave(waveData, SAMPLE_RATE, TONE_HZ, DURATION);

    // �T�E���h�o�b�t�@�̐ݒ�
    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = static_cast<UINT32>(waveData.size());
    buffer.pAudioData = waveData.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    // �o�b�t�@���\�[�X�{�C�X�ɑ��M
    hr = pSourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        std::cerr << "�o�b�t�@�̑��M�Ɏ��s���܂����B" << std::endl;
        return -1;
    }

    IUnknown * pXAPO;
    CreateFX( __uuidof( FXEcho ), & pXAPO );

    XAUDIO2_EFFECT_DESCRIPTOR descriptor;
    descriptor.InitialState = true;
    descriptor.OutputChannels = 1;
    descriptor.pEffect = pXAPO;

    XAUDIO2_EFFECT_CHAIN chain;
    chain.EffectCount = 1;
    chain.pEffectDescriptors = &descriptor;

    pSourceVoice->SetEffectChain( & chain );

    // �Đ��J�n
    hr = pSourceVoice->Start(0);
    if (FAILED(hr)) {
        std::cerr << "�Đ��̊J�n�Ɏ��s���܂����B" << std::endl;
        return -1;
    }

    // �Đ����̑ҋ@
    std::cout << "�T�C���g���Đ���..." << std::endl;
    Sleep(static_cast<DWORD>(10 * 1000));  // 10 �b�ҋ@

    // �I������
    pXAPO->Release();

    pSourceVoice->DestroyVoice();
    pMasterVoice->DestroyVoice();
    pXAudio2->Release();

    ::CoUninitialize();

    return 0;
}
