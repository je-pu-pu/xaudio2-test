#include <xaudio2.h>
#include <xapofx.h>
#include <iostream>
#include <cmath>
#include <vector>

// #define LIBREMIDI_WINMM 1
// #include <libremidi/libremidi.hpp>
#include <libremidi/reader.hpp>
#include <fstream>

#include <chrono>

// #pragma comment(lib, "XAPOFX.lib")

// �萔�ݒ�
const int SAMPLE_RATE = 44100;    // �T���v�����[�g�i44.1kHz�j
const float DURATION = 0.5f;      // �Đ����ԁi�b�j

IXAudio2* pXAudio2 = nullptr;

// �T�C���g�𐶐�����֐�
void GenerateSineWave(std::vector<BYTE>& buffer, int sampleRate, int frequency, float duration) {
    int sampleCount = static_cast<int>(sampleRate * duration);
    buffer.resize(sampleCount * sizeof(float));

    float atack = DURATION * 0.01f;
    float release_time = DURATION * 0.25f;

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

/*
class MyVoiceCallback : public IXAudio2VoiceCallback {
public:
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override {
        // ���̃I�[�f�B�I�o�b�t�@�𐶐����鏈��
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = sampleSize * sizeof(float); // �t���[�g�z��̃T�C�Y
        buffer.pAudioData = reinterpret_cast<BYTE*>(audioDataBuffer);  // �g�`�f�[�^�̃|�C���^
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        // �V�����f�[�^���Đ�����
        sourceVoice->SubmitSourceBuffer(&buffer);
    }

    void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override {
        // �o�b�t�@�̍ċ��������鏈��
        OnVoiceProcessingPassStart(0);
    }

    // ���̃R�[���o�b�N���\�b�h�͕K�v�ɉ����Ď���
};
*/

class SinVoice
{
private:
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    XAUDIO2_BUFFER buffer = { 0 };

public:
    SinVoice( int hz )
    {
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
        buffer.AudioBytes = static_cast<UINT32>(waveData.size());
        buffer.pAudioData = waveData.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;
    }

    ~SinVoice()
    {
        pSourceVoice->DestroyVoice();
    }

    void Play()
    {
        pSourceVoice->Stop();


        // ���ꂪ�厖
        pSourceVoice->FlushSourceBuffers();

        // �o�b�t�@���\�[�X�{�C�X�ɑ��M
        HRESULT hr = pSourceVoice->SubmitSourceBuffer(&buffer);

        if ( FAILED( hr ) )
        {
            throw "�o�b�t�@�̑��M�Ɏ��s���܂����B";
        }

        hr = pSourceVoice->Start( 0 );

        if ( FAILED(hr) )
        {
            throw "�Đ��Ɏ��s���܂����B";
        }
    }

    IXAudio2SourceVoice* getSourceVoice () { return pSourceVoice; }
};

libremidi::midi_track read_midi( const char* midi_file_path )
{
    // Read raw from a MIDI file
    std::ifstream file{ midi_file_path , std::ios::binary };

    std::vector<uint8_t> bytes;
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

    // Initialize our reader object
    libremidi::reader r;

    // Parse
    libremidi::reader::parse_result result = r.parse(bytes);

    if ( result == libremidi::reader::invalid )
    {
        throw "load midi file failed";
    }

    return r.tracks[ 1 ]; // @todo �����g���b�N�Ή�
}

int main()
{
    libremidi::midi_track track = read_midi( "./test.mid" );

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

    {
        auto a = SinVoice( 264 / 2 );
        auto b = SinVoice( 330 / 2 );
        auto c = SinVoice( 396 / 2 );

        IUnknown * pXAPO;
        CreateFX( __uuidof( FXEcho ), & pXAPO );

        XAUDIO2_EFFECT_DESCRIPTOR descriptor;
        descriptor.InitialState = true;
        descriptor.OutputChannels = 1;
        descriptor.pEffect = pXAPO;

        XAUDIO2_EFFECT_CHAIN chain;
        chain.EffectCount = 1;
        chain.pEffectDescriptors = &descriptor;

        a.getSourceVoice()->SetEffectChain( & chain );
        // b.getSourceVoice()->SetEffectChain( & chain );
        // c.getSourceVoice()->SetEffectChain( & chain );

        auto midi_event = track.begin();
        auto last_time = std::chrono::system_clock::now();
        auto tick = std::chrono::milliseconds::zero();

        while ( ( GetAsyncKeyState( VK_ESCAPE ) & 0x8000 ) == 0 )
        {
		    if ( GetAsyncKeyState( 'A' ) & 0x0001 )
            {
                a.Play();
            }
            if ( GetAsyncKeyState( 'S' ) & 0x0001 )
            {
                b.Play();
            }
            if ( GetAsyncKeyState( 'D' ) & 0x0001 )
            {
                c.Play();
            }

            auto now = std::chrono::system_clock::now();
            tick = tick + std::chrono::duration_cast< std::chrono::milliseconds >( now - last_time ) * 2;
            last_time = now;

            for ( ; midi_event != track.end(); midi_event++ )
            {
                for ( ; midi_event != track.end(); midi_event++ )
                {
                    if ( midi_event->m.is_note_on_or_off() )
                    {
                        break;
                    }
                }

                if ( midi_event == track.end() )
                {
                    break;
                }

                if ( tick.count() >= midi_event->tick )
                {
                    if ( midi_event->m.get_message_type() == libremidi::message_type::NOTE_ON && midi_event->m.bytes[ 2 ] > 0 )
                    {
                        if ( midi_event->m.bytes[ 1 ] % 4 >= 2 )
                        {
                            c.Play();
                        }
                        else
                        {
                            b.Play();
                        }
                    }

                    tick -= std::chrono::milliseconds( midi_event->tick );
                }
                else
                {
                    break;
                }
            }

            /*
            if ( midi_event->m.is_note_on_or_off() )
            {
                // std::cout << event.tick << " : " << event.track << " : " <<
                //    ( event.m.get_message_type() == libremidi::message_type::NOTE_ON ? "ON " : "OFF " ) <<
                //    int( event.m.bytes[ 1 ] ) << " : " << int( event.m.bytes[ 2 ] ) << std::endl;
            }
            else
            {
                // std::cout << (int) event.m.bytes[0] << '\n';
            }
            */

		    Sleep( 10 );
	    }

        // �I������
        pXAPO->Release();
    }

    pMasterVoice->DestroyVoice();
    pXAudio2->Release();

    ::CoUninitialize();

    return 0;
}
