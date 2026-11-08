// 0x1e000: JNI ID table teardown
#include "slinky/engine.h"
#include "slinky/jids.h"

// Release the global ref of every class cached by load_all_jni_ids. (The original skips S27PacketExplosion and EntityArrow.)
#define RELEASE(f)                                  \
    do {                                            \
        if (ids->f) {                               \
            (*env)->DeleteGlobalRef(env, ids->f);   \
            ids->f = 0;                             \
        }                                           \
    } while (0)
extern "C" void release_jni_ids(JIds *ids, JNIEnv *env) {
    if (ids->gameClassLoader) {
        (*env)->DeleteGlobalRef(env, (jobject)ids->gameClassLoader);
        ids->gameClassLoader = 0;
    }
    RELEASE(List);
    RELEASE(Map);
    RELEASE(Keyboard);
    RELEASE(Mouse);
    RELEASE(Display);
    RELEASE(Object);
    RELEASE(SecureClassLoader);
    RELEASE(Channel);
    RELEASE(ChannelPipeline);
    RELEASE(Enum);
    RELEASE(Minecraft);
    RELEASE(Entity);
    RELEASE(World);
    RELEASE(Timer);
    RELEASE(GameSettings);
    RELEASE(MovingObjectPosition);
    RELEASE(_init_);
    RELEASE(Block);
    RELEASE(BlockPos);
    RELEASE(KeyBinding);
    RELEASE(AxisAlignedBB);
    RELEASE(GuiContainer);
    RELEASE(EntityLivingBase);
    RELEASE(EntityPlayer);
    RELEASE(InventoryPlayer);
    RELEASE(ItemStack);
    RELEASE(Item);
    RELEASE(Vec3);
    RELEASE(ScorePlayerTeam);
    RELEASE(NetHandlerPlayClient);
    RELEASE(IChatComponent);
    RELEASE(IChatComponent_Serializer);
    RELEASE(Slot);
    RELEASE(IInventory);
    RELEASE(NetworkManager);
    RELEASE(C03PacketPlayer);
    RELEASE(C07PacketPlayerDigging);
    RELEASE(C08PacketPlayerBlockPlacement);
    RELEASE(TileEntity);
    RELEASE(S12PacketEntityVelocity);
    RELEASE(RenderManager);
    RELEASE(S14PacketEntity);
    RELEASE(S18PacketEntityTeleport);
    RELEASE(S19PacketEntityStatus);
    RELEASE(S06PacketUpdateHealth);
    RELEASE(S29PacketSoundEffect);
    RELEASE(EntityOtherPlayerMP);
    RELEASE(GuiInventory);
    RELEASE(EntityPlayerSP);
    RELEASE(EntityRenderer);
    RELEASE(Framebuffer);
    RELEASE(RenderGlobal);
    RELEASE(ActiveRenderInfo);
    RELEASE(RendererLivingEntity);
    RELEASE(FontRenderer);
    RELEASE(GlStateManager);
    RELEASE(C02PacketUseEntity);
    RELEASE(C0BPacketEntityAction);
    RELEASE(S08PacketPlayerPosLook);
    RELEASE(S00PacketKeepAlive);
    RELEASE(C00PacketKeepAlive);
    RELEASE(Potion);
    RELEASE(RenderPlayer);
    RELEASE(RenderItem);
    RELEASE(RenderHelper);
    RELEASE(EntityList);
    RELEASE(Chunk);
    RELEASE(ExtendedBlockStorage);
    RELEASE(WorldClient);
    RELEASE(ChunkProviderClient);
    RELEASE(EntityItem);
    RELEASE(S3FPacketCustomPayload);
    RELEASE(PacketBuffer);
    RELEASE(UUID);
    RELEASE(ChatComponentText);
    RELEASE(PlayerControllerMP);
    RELEASE(C0EPacketClickWindow);
    RELEASE(NBTTagList);
    RELEASE(NBTTagCompound);
    RELEASE(PotionEffect);
    RELEASE(NetworkPlayerInfo);
    RELEASE(GenericFutureListener);
    RELEASE(C16PacketClientStatus);
    RELEASE(C16PacketClientStatus_EnumState);
    RELEASE(c);
    RELEASE(ContainerPlayer);
    RELEASE(GuiScreen);
    RELEASE(GuiChest);
    RELEASE(InventoryBasic);
    RELEASE(StatCollector);
    RELEASE(C01PacketChatMessage);
    RELEASE(C14PacketTabComplete);
    if (ids->S2APacketParticles) {   // last one is a tail call, function pointer loaded first
        auto f = (*env)->DeleteGlobalRef;

        f(env, ids->S2APacketParticles);
    }
}
