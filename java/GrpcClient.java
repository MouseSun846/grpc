package com.example.demo.Config;


import com.example.demo.helloworld.GreeterGrpc;
import com.example.demo.helloworld.HelloReply;
import com.example.demo.helloworld.HelloRequest;
import io.grpc.*;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import io.grpc.stub.StreamObserver;
import org.springframework.stereotype.Component;


@Component
public class GrpcClient {

    private GreeterGrpc.GreeterBlockingStub blockingStub;
    private GreeterGrpc.GreeterStub asyncStub;
    private ManagedChannel channel;

    private StreamObserver<HelloRequest> asyncRequest;

    private AtomicBoolean isReCreated;

    public GrpcClient() {
        isReCreated = new AtomicBoolean(false);
        createChannel();
        new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        Thread.sleep(1000);
                        say();
                    }catch (Exception e){
                        System.out.println(e.getMessage());
                    }
                }
            }
        }).start();

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(1000);
                    asyncReply();
                }catch (Exception e){
                    System.out.println(e.getMessage());
                }
            }
        }).start();
    }
    public void createChannel() {
        channel = ManagedChannelBuilder
                .forAddress("192.168.6.204", 5001)
                .usePlaintext()
                .keepAliveTime(3, TimeUnit.SECONDS)
                .keepAliveTimeout(Long.MAX_VALUE, TimeUnit.DAYS)
                .disableRetry() //开启重试
                .build();

        blockingStub = GreeterGrpc.newBlockingStub(channel);
        asyncStub = GreeterGrpc.newStub(channel);
    }

    public void say() {
        HelloReply reply = blockingStub.sayHello(HelloRequest.newBuilder().setName("lsy"+System.currentTimeMillis()).build());
        System.out.println(reply.getMessage());
    }

    public void asyncReply() {
        asyncRequest = getAsyncRequest();
        while (true) {
            try {
                checkChannelState();
                HelloRequest request = HelloRequest.newBuilder().setName("async=" + System.currentTimeMillis()).build();
                asyncRequest.onNext(request);
//                asyncRequest.onCompleted();
                Thread.sleep(1000);

            }catch (Exception e){
                System.out.println(e.getMessage());
                asyncRequest.onError(e);
            }
        }
    }

    private void checkChannelState() {
        ConnectivityState state = channel.getState(false);
        if(state == ConnectivityState.TRANSIENT_FAILURE || state == ConnectivityState.SHUTDOWN) {
            // 重新创建channel
            createChannel();
            asyncRequest = getAsyncRequest();
            isReCreated.set(false);
            System.out.println("ReCreated channel..");
        }
        if(isReCreated.get()) {
            asyncRequest = getAsyncRequest();
            isReCreated.set(false);
            System.out.println("ReCreated asyncRequest..");
        }
    }

    private StreamObserver<HelloRequest> getAsyncRequest() {
        return asyncStub.asyncRequest(new StreamObserver<HelloReply>() {
            @Override
            public void onNext(HelloReply helloReply) {
                System.out.println("helloReply:" + helloReply.getMessage());
            }

            @Override
            public void onError(Throwable throwable) {
                // 处理StatusRuntimeException异常并在必要时重新创建流
                if (throwable instanceof StatusRuntimeException) {
                    // 重新创建流
                    isReCreated.set(true);
                } else {
                    System.out.println(throwable.getMessage());
                }
            }

            @Override
            public void onCompleted() {
                System.out.println("Finished asyncRequest");
            }
        });
    }

}
