#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "custom-types/shared/register.hpp"
#include <dlfcn.h>

#include "config.hpp"
#include "AssetBundleManager.hpp"
#include "AudioLink.hpp"
#include "Providers/GameProvider.hpp"
#include "Providers/MenuProvider.hpp"

#include "Zenject/DiContainer.hpp"
#include "Zenject/FromBinderNonGeneric.hpp"
#include "Zenject/ConcreteIdBinderGeneric_1.hpp"
#include "GlobalNamespace/StandardGameplayInstaller.hpp"
#include "GlobalNamespace/MissionGameplayInstaller.hpp"
#include "GlobalNamespace/MultiplayerLocalActivePlayerInstaller.hpp"
#include "GlobalNamespace/MainSettingsMenuViewControllersInstaller.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "GlobalNamespace/PCAppInit.hpp"
#include "GlobalNamespace/QuestAppInit.hpp"
#include "GlobalNamespace/ColorManagerInstaller.hpp"
#include "GlobalNamespace/SongPreviewPlayer.hpp"

#include "lapiz/shared/zenject/Zenjector.hpp"

MAKE_HOOK_MATCH(SongPreviewPlayer_CrossFadeTo, static_cast<void (GlobalNamespace::SongPreviewPlayer::*)(::UnityEngine::AudioClip*, float, float, float, bool, ::System::Action*)>(&GlobalNamespace::SongPreviewPlayer::CrossfadeTo), void, GlobalNamespace::SongPreviewPlayer* self, ::UnityEngine::AudioClip* audioClip, float musicVolume, float startTime, float duration, bool isDefault, ::System::Action* onFadeOutCallback) {
    AudioLinkLogger.info("SongPreviewPlayer_CrossFadeTo");
    SongPreviewPlayer_CrossFadeTo(self, audioClip, musicVolume, startTime, duration, isDefault, onFadeOutCallback);
    auto menuProvider = AudioLink::MenuProvider::get_instance();
    if (menuProvider) {
        menuProvider->SongPreviewPlayerProvide(self->_activeChannel, self->_audioSourceControllers);
    } else {
        AudioLinkLogger.info("No menu provider exists!");
    }
}

static void RegisterAudioLinkCapability() {
    // Quest SongCore 1.1.26 exposes RegisterCapability as a C++ string_view
    // symbol. Resolve it exactly like NoodleWrapper does rather than linking
    // against the C++ ABI directly.
    using RegisterCapability_t = void (*)(const char*, __SIZE_TYPE__);

    static constexpr char capability[] = "AudioLink";
    static constexpr __SIZE_TYPE__ capabilityLength = sizeof(capability) - 1;

    static constexpr char symbolNdk[] =
        "_ZN8SongCore3API12Capabilities18RegisterCapabilityENSt6__ndk117basic_string_viewIcNS2_11char_traitsIcEEEE";
    static constexpr char symbolStd[] =
        "_ZN8SongCore3API12Capabilities18RegisterCapabilityESt17basic_string_viewIcSt11char_traitsIcEE";

    void* handle = dlopen("libsongcore.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        AudioLinkLogger.error("Could not load SongCore: {}", dlerror());
        return;
    }

    auto registerCapability = reinterpret_cast<RegisterCapability_t>(dlsym(handle, symbolNdk));
    if (!registerCapability) {
        registerCapability = reinterpret_cast<RegisterCapability_t>(dlsym(handle, symbolStd));
    }

    if (!registerCapability) {
        AudioLinkLogger.error("SongCore RegisterCapability symbol was not found.");
        return;
    }

    registerCapability(capability, capabilityLength);
    AudioLinkLogger.info("Registered AudioLink capability with Quest SongCore.");
}

MAKE_HOOK_MATCH(ColorManagerInstaller_InstallBindings, &GlobalNamespace::ColorManagerInstaller::InstallBindings, void, GlobalNamespace::ColorManagerInstaller* self) {
    AudioLinkLogger.info("ColorManagerInstaller_InstallBindings");
    ColorManagerInstaller_InstallBindings(self);
    RegisterAudioLinkCapability();

    auto menuProvider = AudioLink::MenuProvider::get_instance();
    if (menuProvider) {
        menuProvider->ColorManagerInstallerProvide(self->_menuColorScheme);
    } else {
        AudioLinkLogger.info("No menu provider exists!");
    }
}

MOD_EXTERN_FUNC void setup(CModInfo *info) noexcept {
    *info = modInfo.to_c();
    Paper::Logger::RegisterFileContextId(AudioLinkLogger.tag);
}

MOD_EXTERN_FUNC void late_load() {
    il2cpp_functions::Init();

    if (!LoadConfig()) SaveConfig();
    custom_types::Register::AutoRegister();

    auto& logger = AudioLinkLogger;
    auto zenjector = Lapiz::Zenject::Zenjector::Get();
    zenjector->Install(Lapiz::Zenject::Location::Player, [](::Zenject::DiContainer* container){
        container->BindInterfacesTo<AudioLink::GameProvider*>()->AsSingle()->NonLazy();
    });
    zenjector->Install(Lapiz::Zenject::Location::App, [](::Zenject::DiContainer* container){
        container->BindInterfacesAndSelfTo<AudioLink::AssetBundleManager*>()->AsSingle();
        container->BindInterfacesAndSelfTo<AudioLink::AudioLinkObj*>()->AsSingle();
    });
    zenjector->Install(Lapiz::Zenject::Location::Menu, [](::Zenject::DiContainer* container){
        container->Bind<AudioLink::MenuProvider*>()->AsSingle()->NonLazy();
    });

    INSTALL_HOOK(logger, SongPreviewPlayer_CrossFadeTo);
    INSTALL_HOOK(logger, ColorManagerInstaller_InstallBindings);

    RegisterAudioLinkCapability();
}
