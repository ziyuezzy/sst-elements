// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Needed for serialization
class VirtNic {
    Nic* m_nic;
public:
    VirtNic(Nic* nic, int _id, std::string portName, SimTime_t delay) : m_nic(nic), id(_id), m_msgHostDelay( delay )
    {
        std::ostringstream tmp;
        tmp <<  id;

        m_toCoreLink = m_nic->configureLink( portName + tmp.str(), "1 ns",
                                          new Event::Handler2<Nic::VirtNic,&Nic::VirtNic::handleCoreEvent>(this) );
        assert( m_toCoreLink );
    }
    ~VirtNic() {}

    void handleCoreEvent( Event* ev ) {
        m_nic->handleVnicEvent( ev, id );
    }

    void init( unsigned int phase ) {
        if ( 0 == phase ) {
            m_toCoreLink->sendUntimedData( new NicInitEvent(
                                               m_nic->getNodeId(), id, m_nic->getNum_vNics() ) );
        }
    }

    void send( SST::Event * event ) {
        m_toCoreLink->send( m_msgHostDelay, event );
    }

    void sendShmem( SimTime_t delay, SST::Event * event ) {
        m_toCoreLink->send( delay , event );
    }


    Link* m_toCoreLink;
    int id;
    SimTime_t m_msgHostDelay;
    void notifyRecvDmaDone( int src_vNic, int src, int tag, size_t len,
                            void* key ) {
        send( new NicRespEvent( NicRespEvent::DmaRecv, src_vNic,
                                src, tag, len, key ) );
    }
    void notifyNeedRecv( int src_vNic, int src, size_t len ) {
        send( new NicRespEvent( NicRespEvent::NeedRecv, src_vNic,
                                src, 0, len ) );
    }
    void notifySendDmaDone( void* key ) {
        send( new NicRespEvent( NicRespEvent::DmaSend, key));
    }
    void notifySendPioDone( void* key ) {
        send( new NicRespEvent( NicRespEvent::PioSend, key));
    }
    void notifyPutDone( void* key ) {
        send( new NicRespEvent( NicRespEvent::Put, key ));
    }
    void notifyGetDone( void* key ) {
        send( new NicRespEvent( NicRespEvent::Get, key ));
    }

    void notifyShmem( SimTime_t delay ) {
        sendShmem( delay, new NicShmemRespEvent( [](){} ));
    }

    void notifyShmem( SimTime_t delay, NicShmemRespEvent::Callback callback ) {
        sendShmem( delay, new NicShmemRespEvent( callback ));
    }

    void notifyShmem( SimTime_t delay, NicShmemValueRespEvent::Callback callback, Hermes::Value& value ) {
        sendShmem( delay, new NicShmemValueRespEvent( callback, value ));
    }

    // Following functions needed so checkpointable handlers will
    // compile. They do not currently do anything else usefule, but
    // can be filled in when checkpointing is enabled for this element
    // library.
    void serialize_order(SST::Core::Serialization::serializer& ser) {}
    VirtNic() : m_nic(nullptr) {};

};
